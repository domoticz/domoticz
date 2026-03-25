.PHONY: help deploy create-function create-role add-alexa-permission lambda-status test-discovery test-statereport test-control clean show-current-version zip

# Name of your Lambda function in AWS
FUNCTION_NAME ?= domoticz-alexa
# AWS CLI profile to use (leave empty for default profile)
PROFILE ?=
# AWS region where your Lambda function will be deployed
AWS_REGION ?= eu-west-1
# Your Alexa Skill ID (recommended for security, restricts which skills can invoke your Lambda)
SKILL_ID ?=
# Bearer token for testing (can be set via environment variable)
BEARER_TOKEN ?=

ACCOUNT_ID = $(shell aws sts get-caller-identity $(PROFILE_FLAG) --query Account --output text 2>/dev/null)
ROLE_ARN ?= arn:aws:iam::$(ACCOUNT_ID):role/$(FUNCTION_NAME)-execution-role

ifdef PROFILE
PROFILE_FLAG = --profile $(PROFILE)
else
PROFILE_FLAG =
endif

help:
	@echo "Initial setup:"
	@echo "  First-time setup requires three steps:"
	@echo "  1. Create an IAM role for the Lambda function to run with"
	@echo "  2. Create the Lambda function itself"
	@echo "  3. Add permissions for Alexa to invoke the function"
	@echo ""
	@echo "  create-role          - Create IAM role for Lambda"
	@echo "  create-function      - Create new Lambda function"
	@echo "  add-alexa-permission - Add Alexa Smart Home trigger permission"
	@echo ""
	@echo "Development:"
	@echo "  deploy               - Package and deploy Lambda function"
	@echo "  lambda-status        - Check Lambda function status and configuration"
	@echo "  show-current-version - Show deployed Lambda version"
	@echo ""
	@echo "Testing:"
	@echo "  test-discovery       - Test device discovery (use BEARER_TOKEN=xxx to specify JWT)"
	@echo "  test-statereport     - Test state report (requires ENDPOINT=xxx, optional BEARER_TOKEN=xxx)"
	@echo "  test-control         - Test device control (requires ENDPOINT=xxx, optional BEARER_TOKEN=xxx)"
	@echo "  logs                 - Fetch recent CloudWatch logs"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean                - Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  PROFILE         - AWS profile (default: none)"
	@echo "  FUNCTION_NAME   - Lambda function name (default: domoticz-alexa)"
	@echo "  AWS_REGION      - AWS region (default: eu-west-1)"
	@echo "  ROLE_ARN        - IAM role ARN (default: auto-detected)"
	@echo "  SKILL_ID        - Alexa skill ID (default: none, allows all skills)"

lambda-$(FUNCTION_NAME).zip: lambda_function.py
	$(eval GIT_VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "unknown"))
	@echo "Creating lambda-$(FUNCTION_NAME).zip (version: $(GIT_VERSION))..."
	@zip -q lambda-$(FUNCTION_NAME).zip lambda_function.py

zip: lambda-$(FUNCTION_NAME).zip

deploy: lambda-$(FUNCTION_NAME).zip
	@echo "Deploying Lambda function..."
	$(eval GIT_VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "unknown"))
	@aws lambda update-function-code \
		--function-name $(FUNCTION_NAME) \
		--zip-file fileb://lambda-$(FUNCTION_NAME).zip \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'LastModified' \
		--output text
	@echo "Waiting for function update to complete..."
	@aws lambda wait function-updated \
		--function-name $(FUNCTION_NAME) \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION)
	@aws lambda update-function-configuration \
		--function-name $(FUNCTION_NAME) \
		--description "Domoticz Alexa Smart Home - $(GIT_VERSION)" \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'Description' \
		--output text
	@echo "Deployment complete"

create-role:
	@echo "Creating IAM role for Lambda execution..."
	@echo '{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Principal":{"Service":"lambda.amazonaws.com"},"Action":"sts:AssumeRole"}]}' > /tmp/trust-policy.json
	@aws iam create-role \
		--role-name $(FUNCTION_NAME)-execution-role \
		--assume-role-policy-document file:///tmp/trust-policy.json \
		$(PROFILE_FLAG) \
		--query 'Role.Arn' \
		--output text
	@echo "Creating CloudWatch Logs policy..."
	@echo '{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Action":"logs:CreateLogGroup","Resource":"arn:aws:logs:$(AWS_REGION):$(ACCOUNT_ID):*"},{"Effect":"Allow","Action":["logs:CreateLogStream","logs:PutLogEvents"],"Resource":["arn:aws:logs:$(AWS_REGION):$(ACCOUNT_ID):log-group:/aws/lambda/$(FUNCTION_NAME):*"]}]}' > /tmp/logs-policy.json
	@aws iam put-role-policy \
		--role-name $(FUNCTION_NAME)-execution-role \
		--policy-name CloudWatchLogsPolicy \
		--policy-document file:///tmp/logs-policy.json \
		$(PROFILE_FLAG)
	@rm -f /tmp/trust-policy.json /tmp/logs-policy.json
	@echo "Role created. Use this ARN with create-function:"
	@aws iam get-role \
		--role-name $(FUNCTION_NAME)-execution-role \
		$(PROFILE_FLAG) \
		--query 'Role.Arn' \
		--output text

fix-logs:
	@echo "Adding CloudWatch Logs permissions to Lambda role..."
	@aws iam attach-role-policy \
		--role-name $(shell echo $(ROLE_ARN) | sed 's|.*/||') \
		--policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole \
		$(PROFILE_FLAG)
	@echo "Permissions added. Logs should now work."

create-function: lambda-$(FUNCTION_NAME).zip
	@echo "Creating Lambda function $(FUNCTION_NAME)..."
	@echo "Using role: $(ROLE_ARN)"
	$(eval GIT_VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "unknown"))
	@aws lambda create-function \
		--function-name $(FUNCTION_NAME) \
		--runtime python3.12 \
		--role $(ROLE_ARN) \
		--handler lambda_function.lambda_handler \
		--architectures arm64 \
		--timeout 80 \
		--description "Domoticz Alexa Smart Home - $(GIT_VERSION)" \
		--zip-file fileb://lambda-$(FUNCTION_NAME).zip \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'FunctionArn' \
		--output text
	@echo "Function created successfully"

add-alexa-permission:
ifdef SKILL_ID
	@echo "Adding Alexa Smart Home permission for skill $(SKILL_ID)..."
	$(eval SKILL_SUFFIX := $(shell echo $(SKILL_ID) | rev | cut -d. -f1 | rev))
	@aws lambda add-permission \
		--function-name $(FUNCTION_NAME) \
		--statement-id alexa-smart-home-$(SKILL_SUFFIX) \
		--action lambda:InvokeFunction \
		--principal alexa-connectedhome.amazon.com \
		--event-source-token $(SKILL_ID) \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'Statement' \
		--output text
	@echo "Permission added for skill $(SKILL_ID)"
else
	@echo "Adding Alexa Smart Home permission for all skills..."
	@aws lambda add-permission \
		--function-name $(FUNCTION_NAME) \
		--statement-id alexa-smart-home \
		--action lambda:InvokeFunction \
		--principal alexa-connectedhome.amazon.com \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'Statement' \
		--output text
	@echo "Permission added for all Alexa Smart Home skills"
endif

lambda-status:
	@echo "=== Lambda Function Status ==="
	@echo ""
	@echo "Checking IAM role..."
	@aws iam get-role --role-name $(shell echo $(ROLE_ARN) | sed 's|.*/||') $(PROFILE_FLAG) --query 'Role.{Name:RoleName,Arn:Arn}' --output table 2>/dev/null || echo "Role not found: $(ROLE_ARN)"
	@echo ""
	@echo "Checking Lambda function..."
	@aws lambda get-function-configuration --function-name $(FUNCTION_NAME) $(PROFILE_FLAG) --region $(AWS_REGION) --query '{Name:FunctionName,Runtime:Runtime,Role:Role,Description:Description,LastModified:LastModified}' --output table 2>/dev/null || echo "Function not found: $(FUNCTION_NAME)"
	@echo ""
	@echo "Checking Alexa permissions..."
	@aws lambda get-policy --function-name $(FUNCTION_NAME) $(PROFILE_FLAG) --region $(AWS_REGION) --query 'Policy' --output text 2>/dev/null | jq -r '.Statement[] | select(.Principal.Service == "alexa-connectedhome.amazon.com") | "StatementId: \(.Sid)\nPrincipal: \(.Principal.Service)\nEventSourceToken: \(.Condition.StringEquals["lambda:EventSourceToken"] // "all skills")"' 2>/dev/null || echo "No Alexa permissions found"

test-discovery:
	@if [ -z "$(BEARER_TOKEN)" ]; then \
		echo "Error: BEARER_TOKEN required. Set via environment or command line."; \
		exit 1; \
	fi
	@echo "Testing device discovery..."
	@echo '{"directive":{"header":{"namespace":"Alexa.Discovery","name":"Discover","payloadVersion":"3","messageId":"test"},"payload":{"scope":{"type":"BearerToken","token":"$(BEARER_TOKEN)"}}}}' > discovery-payload.json
	@aws lambda invoke \
		--function-name $(FUNCTION_NAME) \
		--payload file://discovery-payload.json \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		discovery-response.json > /dev/null
	@jq '.event.payload.endpoints | length' discovery-response.json | \
		xargs -I {} echo "Discovered {} devices"
	@jq -r '.event.payload.endpoints[].friendlyName' discovery-response.json | sort

test-statereport:
	@if [ -z "$(ENDPOINT)" ]; then \
		echo "Error: ENDPOINT required. Usage: make test-statereport ENDPOINT=<endpointId>"; \
		exit 1; \
	fi
	@if [ -z "$(BEARER_TOKEN)" ]; then \
		echo "Error: BEARER_TOKEN required. Set via environment or command line."; \
		exit 1; \
	fi
	@echo "Testing state report for endpoint $(ENDPOINT)..."
	@if [ ! -f discovery-response.json ]; then \
		echo "Running discovery first to get device cookie..."; \
		$(MAKE) test-discovery > /dev/null 2>&1; \
	fi
	@COOKIE=$$(jq -c '.event.payload.endpoints[] | select(.endpointId == "$(ENDPOINT)") | .cookie' discovery-response.json 2>/dev/null || echo '{}'); \
	echo "{\"directive\":{\"header\":{\"namespace\":\"Alexa\",\"name\":\"ReportState\",\"payloadVersion\":\"3\",\"messageId\":\"test\",\"correlationToken\":\"test\"},\"endpoint\":{\"scope\":{\"type\":\"BearerToken\",\"token\":\"$(BEARER_TOKEN)\"},\"endpointId\":\"$(ENDPOINT)\",\"cookie\":$$COOKIE},\"payload\":{}}}" > statereport-payload.json
	@aws lambda invoke \
		--function-name $(FUNCTION_NAME) \
		--payload file://statereport-payload.json \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		statereport-response.json > /dev/null
	@echo "Response:"
	@jq '.' statereport-response.json

test-control:
	@if [ -z "$(ENDPOINT)" ]; then \
		echo "Error: ENDPOINT required. Usage: make test-control ENDPOINT=<endpointId>"; \
		exit 1; \
	fi
	@if [ -z "$(BEARER_TOKEN)" ]; then \
		echo "Error: BEARER_TOKEN required. Set via environment or command line."; \
		exit 1; \
	fi
	@echo "Testing device control (TurnOn) for endpoint $(ENDPOINT)..."
	@if [ ! -f discovery-response.json ]; then \
		echo "Running discovery first to get device cookie..."; \
		$(MAKE) test-discovery > /dev/null 2>&1; \
	fi
	@COOKIE=$$(jq -c '.event.payload.endpoints[] | select(.endpointId == "$(ENDPOINT)") | .cookie' discovery-response.json 2>/dev/null || echo '{}'); \
	echo "{\"directive\":{\"header\":{\"namespace\":\"Alexa.PowerController\",\"name\":\"TurnOn\",\"payloadVersion\":\"3\",\"messageId\":\"test\",\"correlationToken\":\"test\"},\"endpoint\":{\"scope\":{\"type\":\"BearerToken\",\"token\":\"$(BEARER_TOKEN)\"},\"endpointId\":\"$(ENDPOINT)\",\"cookie\":$$COOKIE},\"payload\":{}}}" > control-payload.json
	@aws lambda invoke \
		--function-name $(FUNCTION_NAME) \
		--payload file://control-payload.json \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		control-response.json > /dev/null
	@jq '.' control-response.json

logs:
	@aws logs filter-log-events \
		--log-group-name /aws/lambda/$(FUNCTION_NAME) \
		--start-time $$(date -d '5 minutes ago' +%s)000 \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'events[*].message' \
		--output text

show-current-version:
	@aws lambda get-function-configuration \
		--function-name $(FUNCTION_NAME) \
		$(PROFILE_FLAG) \
		--region $(AWS_REGION) \
		--query 'Description' \
		--output text

clean:
	rm -f discovery-payload.json discovery-response.json control-payload.json control-response.json statereport-payload.json statereport-response.json lambda-$(FUNCTION_NAME).zip
