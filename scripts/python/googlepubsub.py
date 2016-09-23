import httplib2
import argparse
import base64
import csv
import datetime
import random
import sys
import time
import base64
import domoticz

# This script connects Domoticz with Google Cloud PubSub API
# To make it works :
# 1 - Install the Google API Client Python Library : pip install --upgrade google-api-python-client (or see https://cloud.google.com/pubsub/libraries)
# 2 - Activate the Google Cloud PubSub API in the Google Cloud Console
# 3 - Generate a Service account key in the Google Cloud Console and download the private key in JSON format
# 4 - Transfer the json key into the domoticz install dir
# 5 - Set the ENV variable GOOGLE_APPLICATION_CREDENTIALS to the path of your Service account key
# 6 - Create a topic in the "PubSub" Google Cloud Console
# 7 - Modify the variable PUBSUB_TOPICNAME below with the topic name

# Required library to make pubsub google api working fine
from apiclient import discovery
from oauth2client import client as oauth2client

# Required privileges
PUBSUB_SCOPES = ['https://www.googleapis.com/auth/pubsub']

# Topic name defined in the Google Cloud PubSub Console
PUBSUB_TOPICNAME = 'projects/affable-enigma-136123/topics/domoticz'

def create_pubsub_client(http=None):
    credentials = oauth2client.GoogleCredentials.get_application_default()
    if credentials.create_scoped_required():
        credentials = credentials.create_scoped(PUBSUB_SCOPES)
    if not http:
        http = httplib2.Http()
    credentials.authorize(http)

    return discovery.build('pubsub', 'v1', http=http)

def publish_message(client,topicname,message):
        message1 = base64.b64encode(message)
        body = {
                'messages': [
                {'data': message1}
                ]
        }
        resp = client.projects().topics().publish(topic=topicname, body=body).execute()

def main(argv):
        client = create_pubsub_client()
        publish_message(client,PUBSUB_TOPICNAME,data)

if __name__ == '__main__':
        main(sys.argv)

