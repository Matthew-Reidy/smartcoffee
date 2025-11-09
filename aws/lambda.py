import json
import boto3
import os

client = boto3.client('iot-data')

API_CLIENT =  boto3.client('apigatewaymanagementapi', endpoint_url=os.getenv("CALLBACK_URL"))

def lambda_handler(event, context):

    connectionId : str = event["requestContext"]["connectionId"]

    coffeemaker = "coffeemaker/topic"

    client.publish(topic=coffeemaker, payload="my message")

    
    match event["requestContext"]["eventType"]:
        
        case "CONNECT":

            sendMessage(json.dumps({"message": "hello"}), connectionId)
            
         
        case "MESSAGE":

            print("valid session...distributing messages")
            sendMessage(event["body"], connectionId)

    return {
        "statusCode": 200
    }


def sendMessage(body, connectID):

    try:
       
        API_CLIENT.post_to_connection(
                                    Data=json.dumps(body).encode('utf-8'), 
                                    ConnectionId=connectID        
                                    )
            
    except Exception as e:
        print(e)

