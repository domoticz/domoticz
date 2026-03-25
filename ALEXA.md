# Domoticz Alexa Smart Home Integration

This integration allows you to control your Domoticz devices through Amazon Alexa using the Alexa Smart Home API v3.

Domoticz includes native Alexa Smart Home API support and a built-in OAuth2 server that works with Alexa's account linking. A minimal AWS Lambda function acts as a passthrough, forwarding Alexa requests to your Domoticz instance.

## Features

- Control lights (on/off, dimming, color, color temperature)
- Control scenes and groups
- Control selector switches (mode controllers)
- Monitor temperature and humidity sensors
- Control thermostats
- Control blinds and locks
- Monitor various sensors (weight, pH, absolute humidity)

## Requirements

- **Domoticz home automation system** - Your Domoticz instance must be accessible from the Lambda function over Legacy IP, with a valid SSL certificate.
- **Amazon Developer Account** - Sign up at [developer.amazon.com](https://developer.amazon.com) to create your Alexa Smart Home skill
- **AWS Account** - Needed to host the Lambda function that bridges Alexa and Domoticz. AWS Lambda offers a free tier with 1 million requests per month, which is more than sufficient for typical home automation use.

## Installation

### Prepare Domoticz

Before setting up the Alexa skill, ensure your Domoticz instance is properly configured:

1. **Network accessibility** - Your Domoticz instance must be accessible from the internet via Legacy IP, as Lambda functions cannot use IPv6 without additional VPC configuration. You may need to configure port forwarding on your router.

   Advanced users who don't want to open a public port can configure the Lambda function to run in their own specific VPC (private network), and can configure their local network to connect to that VPC via a VPN using the methods in [the AWS documentation](https://docs.aws.amazon.com/vpc/latest/userguide/vpn-connections.html). Such a configuration is outside the scope of this document.

2. **Valid SSL certificate** - Domoticz must use HTTPS with a certificate from a recognized Certificate Authority. Self-signed certificates will not work. LetsEncrypt certificates work well and are free.

3. **Create a dedicated Alexa user** (recommended):
   - Go to Setup → Settings → Users
   - Create a new user (e.g., "alexa")
   - Set user rights to at least "User" if you want Alexa to control devices. With only "Viewer" permissions, Alexa will be able to see devices but not change anything.
   - Grant access only to the devices you want to control via Alexa. If you don't explicitly add any devices to this user, Alexa will have access to everything, which is probably not what you want.
   - This limits the scope of what Alexa can access

4. **Configure OAuth2 application**:
   - Go to More Options → Applications
   - Enter Application Name: `domoticz-alexa` (or your preferred name)
   - Enter Application Secret: Generate a secure random string (save this for later)
   - Leave "Is Public" switch OFF
   - Click "Add" and you'll see it added to the list
   - Note the Application Name and Application Secret (these will be the Client ID and Client Secret you'll need for Alexa account linking)

### Create an Alexa Smart Home Skill

1. Log in to the [Alexa Developer Console](https://developer.amazon.com/alexa/console/ask)
2. Click "Create Skill"
3. Configure the skill:
   - Skill name: Choose a name (e.g., "Domoticz")
   - Primary locale: English (US) or your preferred locale
   - Choose a model: Smart Home
   - Choose a method to host: Provision your own
4. Click "Create skill"
5. Note your Skill ID (you'll need this later)

Leave this browser tab open — you'll return to complete the skill setup after creating the Lambda function.

### Create an AWS Lambda Function

**NOTE:** The Lambda function must be created in the same AWS region as your Alexa account, or the skill will not work. The required region depends on your Amazon account's PFM (Preferred Marketplace), which can be found in your retail Amazon account settings. See [this Amazon blog post](https://developer.amazon.com/en-US/blogs/alexa/device-makers/2023/07/smart-home-discovery-alexa-july-2023) for more information about PFM and regional requirements.

It's easiest to create the Lambda function using the Makefile if you have the AWS CLI configured, or you can do it through the AWS Console in your web browser.

**Using the Makefile:**

1. Configure AWS CLI credentials (see [AWS CLI Quick Start](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-quickstart.html)). For a personal setup, it's acceptable to use [user credentials](https://docs.aws.amazon.com/cli/latest/userguide/cli-authentication-user.html). The IAM user or role will need the following permissions:
   - `iam:CreateRole`, `iam:GetRole`, `iam:PutRolePolicy` - to create the Lambda execution role
   - `lambda:CreateFunction`, `lambda:UpdateFunctionCode`, `lambda:UpdateFunctionConfiguration`, `lambda:GetFunction`, `lambda:AddPermission` - to create and manage the Lambda function
   - `sts:GetCallerIdentity` - to determine your AWS account ID
   
   These permissions are included in the AWS managed policies `IAMFullAccess` and `AWSLambda_FullAccess`, or you can create a custom policy with only the specific permissions listed above.

2. Edit the `Makefile` in the `alexa/` directory:
   - Set `REGION` to the same AWS region as your Alexa account
   - Set `SKILL_ID` to your Alexa Skill ID (recommended for better security, but not strictly required)

3. Run the setup commands from the `alexa/` directory:
   ```bash
   cd alexa
   make create-role
   make create-function
   make add-alexa-permission
   ```

To deploy code updates later, you can run:
```bash
make deploy
```

**Using the AWS Console:**

1. Log in to the [AWS Console](https://console.aws.amazon.com/)
2. Select the same AWS region as your Alexa account using the region selector in the top-right corner
3. Navigate to Lambda and click "Create function"
4. Choose "Author from scratch"
5. Configure the function:
   - Function name: `domoticz-alexa` (or your preferred name)
   - Runtime: Python 3.13 (or newer, if available)
   - Architecture: arm64
   - Execution role: Create a new role with basic Lambda permissions
6. Click "Create function"
7. In the Configuration tab:
   - Set Timeout to 80 seconds
   - (Optional) Under Environment variables, add `DOMOTICZ_URL` with your Domoticz server URL (e.g., `https://your-domoticz-host.example.com`) if you want to override the URL extracted from the JWT token
8. In the Code tab, copy the contents of `alexa/lambda_function.py` and paste it into the code editor, replacing the default code
9. Click "Deploy" to save the code
10. Click "Add trigger"
11. Select "Alexa" then "Alexa Smart Home"
12. Enter your Skill ID (found in the Alexa Developer Console under "Skill ID")
13. Click "Add"
14. Note your Lambda function ARN (you'll need this later)

### Complete the Skill Setup

Return to the Alexa Developer Console browser tab from earlier. You should be in the Build tab.

1. Go to "Smart Home"
2. Ensure "Payload version" is set to "v3" (this is the default)
3. Under "Default endpoint", enter your Lambda function ARN
4. Click "Save"
5. Go to your skill's "Account Linking" section
6. Configure the OAuth settings to use Domoticz's built-in OAuth2 server:
   - Authorization URI: `https://your-domoticz-host.example.com/oauth2/v1/authorize`
   - Access Token URI: `https://your-domoticz-host.example.com/oauth2/v1/token`
   - Client ID: Same as configured in Domoticz (e.g., `domoticz-alexa`)
   - Client Secret: Same as configured in Domoticz
   - Authentication Scheme: Credentials in request body
   - Scope: `profile` (or any value — Domoticz doesn't use scopes but Alexa requires one)
7. Click "Save"
8. (Optional) Go to your skill's "Permissions" section and enable "Send Alexa Events". We do not support this yet, but we hope to add it in future.

### Enable the Skill

1. Open the Alexa app on your phone
2. Go to Skills & Games → Your Skills → Dev
3. Find your skill and click "Enable to Use"
4. Complete the account linking process
5. Alexa will discover your Domoticz devices

### Discover Devices

If you add devices which are visible to Alexa, you can say "Alexa, discover devices" or use the Alexa app:
1. Open the Alexa app
2. Go to Devices
3. Tap the "+" icon
4. Select "Add Device"
5. Choose "Other" and follow the prompts

Your Domoticz devices should now appear in Alexa!

## Troubleshooting

### Account linking fails

**If you don't reach the Domoticz login page:**
- Check that your Domoticz server is accessible from the Internet
- Verify your SSL certificate is valid and from a recognized Certificate Authority
- Visit `https://your-domoticz-host.example.com/.well-known/openid-configuration` from an external network to verify it's reachable

**If login appears to succeed but linking fails:**
- Check the `issuer` field in `https://your-domoticz-host.example.com/.well-known/openid-configuration`
- If the `issuer` doesn't match your actual server URL, Domoticz is putting the wrong issuer into tokens
- Set the `DOMOTICZ_URL` environment variable in your Lambda function configuration to override it (e.g., `https://your-domoticz-host.example.com`)

### No devices discovered
- Check your Lambda logs in CloudWatch to see what functions are being invoked
- If you only see an `AcceptGrant` call but no `Discover` call, your Lambda function is likely in the wrong AWS region
- Note: Alexa doesn't report this error correctly - it will appear as if linking succeeded, but no devices will be discovered (thanks, Amazon!)
- Verify your Lambda function is in the same AWS region as your Alexa account (see the PFM note in the Lambda setup section)

### Device not discovered
- Check that the device is marked as "Used" in Domoticz
- Verify the Alexa user has access to the device (for non-admin users)
- Check Lambda CloudWatch logs for errors

### Voice commands not working
- Ensure device names are clear and distinct
- Check that the device type is supported
- Verify the skill is enabled in the Alexa app
- Note: Alexa's natural language understanding (NLU) can be inconsistent. Try opening the device in the Alexa app and controlling it directly to determine if the issue is with the Smart Home integration or just Alexa's voice recognition

### State not updating
- State updates require polling (every 3 seconds when device is open in app)
- Check Domoticz is accessible from Lambda
- Review CloudWatch logs for errors

## Supported Device Types

### Lights
- PowerController (on/off)
- BrightnessController (dimming)
- ColorController (RGB color)
- ColorTemperatureController (white temperature)

### Scenes and Groups
- SceneController (activate scenes)
- PowerController (on/off for groups)

### Selector Switches
- ModeController with custom modes based on Domoticz LevelNames

### Temperature Sensors
- TemperatureSensor

### Humidity Sensors
- TemperatureSensor and HumiditySensor (combined)

### Thermostats
- ThermostatController (set target temperature)
- TemperatureSensor (current temperature)

### Blinds
- RangeController (position control with open/close presets)

### Locks
- LockController

### Other Sensors
- RangeController (weight, pH, absolute humidity)
- PercentageController (percentage sensors)

## Voice Control Examples

### Lights
- "Alexa, turn on the kitchen light"
- "Alexa, dim the bedroom light to 50%"
- "Alexa, set the living room light to blue"

### Scenes
- "Alexa, activate bedtime"
- "Alexa, turn on movie time"

### Selector Switches
- "Alexa, set the thermostat to heat mode"
- "Alexa, set lounge source to HDMI 1"

### Thermostats
- "Alexa, set kitchen temperature to 20 degrees"
- "Alexa, what is the kitchen temperature?"
- "Alexa, what is the kitchen set to?"

### Blinds
- "Alexa, open the bedroom blind"
- "Alexa, close the living room blind"
- "Alexa, set the office blind to 50 percent"
- "Alexa, stop the landing blinds"

### Sensors
- "Alexa, what's the temperature in the bedroom?"

## Development

### Getting a Bearer Token for Testing

To test the Lambda function, you need a JWT bearer token from Domoticz. You can obtain one using the OAuth2 token endpoint:

```bash
curl -X POST https://your-domoticz-host.example.com/oauth2/v1/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=password&username=alexa&password=your-password&client_id=domoticz-alexa&client_secret=your-client-secret"
```

The response will include an `access_token` field containing the JWT bearer token. Export this token for testing:

```bash
export BEARER_TOKEN="eyJhbGc..."
```

### Local Testing

Test discovery:
```bash
cd alexa
make test-discovery
```

Test device control:
```bash
make test-control ENDPOINT=device-id
```

### Logging

CloudWatch logs show:
- Incoming Alexa requests
- Domoticz URL being forwarded to
- HTTP response status
- Errors

View logs:
```bash
cd alexa
make logs
```
