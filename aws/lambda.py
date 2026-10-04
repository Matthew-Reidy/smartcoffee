import json
import boto3
import os
import logging

logger = logging.getLogger()
logger.setLevel(logging.INFO)

# --- env vars (set these in Lambda configuration) ---
# CALLBACK_URL  : https://<api-id>.execute-api.<region>.amazonaws.com/<stage>
# IOT_TOPIC     : MQTT topic the Arduino subscribes to (default: coffee/brew)
# DDB_TABLE     : DynamoDB table name for storing active connection IDs (optional)
#                 Table schema: partition key "connectionId" (String)

CALLBACK_URL  = os.getenv("CALLBACK_URL")
IOT_TOPIC     = os.getenv("IOT_TOPIC", "coffee/brew")
DDB_TABLE     = os.getenv("DDB_TABLE")

iot_client  = boto3.client("iot-data")
apigw       = boto3.client("apigatewaymanagementapi", endpoint_url=CALLBACK_URL)
connections = boto3.resource("dynamodb").Table(DDB_TABLE) if DDB_TABLE else None


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def lambda_handler(event, context):
    ctx        = event.get("requestContext", {})
    event_type = ctx.get("eventType")
    conn_id    = ctx.get("connectionId")

    if event_type == "CONNECT":
        return on_connect(conn_id)

    if event_type == "DISCONNECT":
        return on_disconnect(conn_id)

    if event_type == "MESSAGE":
        return on_message(event, conn_id)

    # No requestContext → invoked directly by an IoT Core rule (Arduino → app)
    return on_iot_event(event)


# ---------------------------------------------------------------------------
# WebSocket lifecycle handlers
# ---------------------------------------------------------------------------

def on_connect(conn_id):
    if connections:
        connections.put_item(Item={"connectionId": conn_id})
    send(conn_id, {"event": "connected"})
    logger.info("CONNECT %s", conn_id)
    return {"statusCode": 200}


def on_disconnect(conn_id):
    if connections:
        connections.delete_item(Key={"connectionId": conn_id})
    logger.info("DISCONNECT %s", conn_id)
    return {"statusCode": 200}


# ---------------------------------------------------------------------------
# Incoming WebSocket message (iOS app → Lambda → IoT Core → Arduino)
# ---------------------------------------------------------------------------

def on_message(event, conn_id):
    try:
        body = json.loads(event.get("body") or "{}")
    except (json.JSONDecodeError, TypeError):
        logger.warning("Non-JSON body from %s: %s", conn_id, event.get("body"))
        send(conn_id, {"event": "error", "detail": "invalid JSON"})
        return {"statusCode": 400}

    action = body.get("action")
    state  = body.get("state")

    if action == "brew" and state in ("on", "off"):
        # Map iOS state → Arduino MQTT message
        mqtt_message = "start" if state == "on" else "stop"
        mqtt_payload = json.dumps({"message": mqtt_message})

        logger.info("Publishing to %s: %s", IOT_TOPIC, mqtt_payload)
        iot_client.publish(
            topic=IOT_TOPIC,
            qos=0,
            payload=mqtt_payload.encode("utf-8"),
        )

        # Acknowledge back to the iOS app
        send(conn_id, {"event": "brew", "state": state, "status": "ok"})

    elif action == "ping":
        send(conn_id, {"event": "pong"})

    else:
        logger.warning("Unknown message from %s: %s", conn_id, body)
        send(conn_id, {"event": "error", "detail": f"unknown action: {action}"})

    return {"statusCode": 200}


# ---------------------------------------------------------------------------
# IoT Core → Lambda (Arduino publishes status → Lambda pushes to app)
# To wire this up: create an IoT Core rule that triggers this Lambda when
# a message arrives on "coffee/status" (or whichever topic the Arduino uses).
# ---------------------------------------------------------------------------

def on_iot_event(event):
    logger.info("IoT event: %s", event)

    if not connections:
        logger.info("No DDB table configured — skipping WebSocket broadcast")
        return {"statusCode": 200}

    # Broadcast the IoT payload to every active WebSocket connection
    result   = connections.scan(ProjectionExpression="connectionId")
    stale    = []

    for item in result.get("Items", []):
        cid = item["connectionId"]
        try:
            send(cid, {"event": "status", "data": event})
        except apigw.exceptions.GoneException:
            stale.append(cid)
        except Exception as e:
            logger.error("Failed to send to %s: %s", cid, e)

    # Clean up connections that are no longer alive
    for cid in stale:
        logger.info("Removing stale connection: %s", cid)
        connections.delete_item(Key={"connectionId": cid})

    return {"statusCode": 200}


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def send(conn_id, data: dict):
    """Send a JSON message to a WebSocket client."""
    apigw.post_to_connection(
        Data=json.dumps(data).encode("utf-8"),
        ConnectionId=conn_id,
    )
