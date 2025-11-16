import json
import urllib3
import base64
import os

http = urllib3.PoolManager()

def lambda_handler(event, context):
    print(f"Request: {event.get('directive', {}).get('header', {}).get('namespace')} / {event.get('directive', {}).get('header', {}).get('name')}")
    
    # Extract bearer token
    bearer_token = (event.get('directive', {}).get('endpoint', {}).get('scope', {}).get('token') or
                   event.get('directive', {}).get('payload', {}).get('scope', {}).get('token') or
                   event.get('directive', {}).get('payload', {}).get('grantee', {}).get('token'))
    
    if not bearer_token:
        print("Error: No bearer token found")
        return {'event': {'header': {'namespace': 'Alexa', 'name': 'ErrorResponse'}, 
                         'payload': {'type': 'INVALID_AUTHORIZATION_CREDENTIAL'}}}
    
    # Decode JWT to get issuer
    try:
        payload = json.loads(base64.urlsafe_b64decode(bearer_token.split('.')[1] + '=='))
        domoticz_url = payload['iss'].rstrip('/') + '/alexa.htm'
        print(f"URL from JWT issuer: {domoticz_url}")
    except Exception as e:
        print(f"Error decoding JWT: {e}")
        return {'event': {'header': {'namespace': 'Alexa', 'name': 'ErrorResponse'}, 
                         'payload': {'type': 'INVALID_AUTHORIZATION_CREDENTIAL'}}}
    
    # Override with environment variable if set
    env_url = os.environ.get('DOMOTICZ_URL')
    if env_url:
        domoticz_url = env_url.rstrip('/')
        if not domoticz_url.endswith('/alexa.htm'):
            domoticz_url += '/alexa.htm'
        print(f"Overriding with DOMOTICZ_URL from environment: {domoticz_url}")
    
    # Forward request to Domoticz
    try:
        response = http.request('POST', domoticz_url,
                               body=json.dumps(event),
                               headers={'Authorization': f'Bearer {bearer_token}',
                                       'Content-Type': 'application/json'})
        print(f"Response status: {response.status}")
        return json.loads(response.data)
    except Exception as e:
        print(f"Error forwarding request: {e}")
        return {'event': {'header': {'namespace': 'Alexa', 'name': 'ErrorResponse'}, 
                         'payload': {'type': 'INTERNAL_ERROR', 'message': str(e)}}}
