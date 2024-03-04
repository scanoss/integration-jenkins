
# Integrating SCANOSS with Jenkins

The following guide provides a basic setup example on integrating SCANOSS with Jenkins. This repository contains an example pipeline, capable of pulling a Github repository, scanning the source code with the [SCANOSS.PY](https://github.com/scanoss/scanoss.py) CLI, and creating an issue on Jira with files containing copyleft licenses. Additionally, a report is shown in the dashboard.

## Pre-requisites

The following Jenkins plugins need to be installed for the full set of features in the example pipeline:

- Docker Pipeline
- Nested Data Reporting
- Pipeline Utility Steps
- Generic Webhook Trigger
- Pipeline
- Github Plugin


## How to configure Jenkins integration

The recommended approach to integrate SCANOSS with Jenkins is to load the pipeline from a 'Jenkinsfile' file.  To load a pipeline from a file:

1. Copy the example `Jenkinsfile` from this repository to the root of your repository.
2. Configure your Jenkins project as follows:

-   Pipeline Definition
    -   Select “Pipeline script from SCM”
        -   SCM: Git
        -   Set the following values:
            -   “Repository URL”
            -   “Credential” (for private repositories)
            -   Enter Branch to build
            -   Set script path as “Jenkinsfile”


### Trigger the pipeline from code repository

A GitHub webhook is used to trigger the pipeline on every push action. 

 - Navigate to Dashboard > Select your pipeline > Configure. 
 - On the Build Triggers section, select the Generic Webhook Trigger option to see the Jenkins webhook URL and copy it. 
 - On Post content parameters, add a new variable. Variable name: 'payload' Value: '$'. Expression type: JSONPath
 - Assign a token to the trigger. The token should be use in the webhook url. For instance **http://JENKINS_URL/generic-webhook-trigger/invoke?token=scanoss**
    
-   Finally set the Jenkins webkook URL on your Github project. For further details check the [Github webhook documentation](https://docs.github.com/en/webhooks/using-webhooks/creating-webhooks "https://docs.github.com/en/webhooks/using-webhooks/creating-webhooks").


## Scan result configuration

In our example pipeline, we process the scan output for license compliance. The results are reported to JIRA (optional) and into the Jenkins bult-in Dashboard using the external plugin "Nested Data Reporting".

NOTE: Some steps may require custom credentials/secrets. For example: Reporting issues to JIRA or cloning a Private Github Repository.

### Jira Integration (optional)

In order to create issues in Jira, you must provide user credentials. New issues will be created in the name of the specified user. 

An API Token is required to integrate Jira. For further details, check the [Jira Documentation](https://support.atlassian.com/atlassian-account/docs/manage-api-tokens-for-your-atlassian-account/).

### Private GitHub Integration (optional)

In case of accessing private GitHub repositories, it is necessary to provide user credentials. For further details, check [Github Documentation](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens).

### Jenkins Credentials

To improve security, credentials should be set in Jenkins credential store. 

Navigate to Dashboard > Manage Jenkins > Credentials and select a credential store (e.g., global) and then create a new credential.

Use the same example pipeline, set the following ids:

- jira-token: JIRA Token to report issues. Type: Secret text
- gh-token: GitHub crendentials to access private repositories. Type: user&password
- scanoss-token: SCANOSS Premium subscription Key. Type: Secret text

### Configuration Options

The following parameters are available in the example pipeline.

| Parameter                | Description                              | Default  | Type    |
|--------------------------|------------------------------------------|----------|---------|
| SCANOSS_CLI_DOCKER_IMAGE | SCANOSS CLI Docker Image                 | http://ghcr.io/scanoss/scanoss-py:latest   | Pipeline |
| ABORT_ON_POLICY_FAILURE  | Abort Pipeline on pipeline Failure       | false    | Pipeline |
| ENABLE_DELTA_ANALYSIS    | Analyze those files what have changed or new ones | false    | Pipeline |
| SCANOSS_API_URL          | SCANOSS API endpoint (Global)            | https://osskb.org/api/scan/direct | Global |
| SCANOSS_API_TOKEN_ID     | SCANOSS API Token ID.                    | scanoss-token | Pipeline |
| SCANOSS_SBOM_IDENTIFY    | SCANOSS SBOM Identify filename           | sbom.json | Pipeline |
| SCANOSS_SBOM_IGNORE      | SCANOSS SBOM Ignore filename             | sbom-ignore.json | Pipeline |
| GITHUB_TOKEN_ID          | Github Repository Token Credential ID.   | gh-token  | Pipeline |
| CREATE_JIRA_ISSUE        | Enables Jira reporting                   | false    | Pipeline |
| JIRA_URL                 | Jira URL                                 |          | Pipeline |
| JIRA_PROJECT_KEY         | Jira Project Key                         |          | Pipeline |
| JIRA_TOKEN_ID            | Jira Token Credential ID                 |  jira-token  | Pipeline |
