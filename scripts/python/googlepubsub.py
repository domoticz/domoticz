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

from apiclient import discovery
from oauth2client import client as oauth2client

PUBSUB_SCOPES = ['https://www.googleapis.com/auth/pubsub']

PUBSUB_TOPICNAME = 'projects/affable-enigma-136123/topics/domoticz'

def create_pubsub_client(http=None):
    credentials = oauth2client.GoogleCredentials.get_application_default()
    if credentials.create_scoped_required():
        credentials = credentials.create_scoped(PUBSUB_SCOPES)
    if not http:
        http = httplib2.Http()
    credentials.authorize(http)

    return discovery.build('pubsub', 'v1', http=http)


def create_topic(client,topicname):
        topic = client.projects().topics().create(name=topicname, body={}).execute()
        print 'Created: %s' % topic.get('name')

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
#       create_topic(client,'projects/affable-enigma-136123/topics/domoticz')
        publish_message(client,PUBSUB_TOPICNAME,data)

if __name__ == '__main__':
        main(sys.argv)

