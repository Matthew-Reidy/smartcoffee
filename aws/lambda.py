import json
import boto3

client = boto3.client('iot-data')

def lambda_handler(event, context):
    
    coffeemaker = "coffeemaker/topic"

    client.publish(topic=coffeemaker, payload="my message")
    
    return {
        'statusCode': 200,
        'body': json.dumps('Hello from Lambda!')
    }
