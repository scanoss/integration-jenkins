pipeline {
    parameters {

        // REPOSITORY variables
        string(name: 'GITHUB_TOKEN_ID', defaultValue:"gh-token", description: 'Github repository token credential id')

        // SCAN Variables
        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')

        string(name: 'SCANOSS_SBOM_IDENTIFY', defaultValue:"sbom.json", description: 'SCANOSS SBOM Identify filename')

        string(name: 'SCANOSS_SBOM_IGNORE', defaultValue:"sbom-ignore.json", description: 'SCANOSS SBOM Ignore filename')

        string(name: 'SCANOSS_CLI_DOCKER_IMAGE', defaultValue:"ghcr.io/scanoss/scanoss-py:latest", description: 'SCANOSS CLI Docker Image')

        booleanParam(name: 'ENABLE_DELTA_ANALYSIS', defaultValue: false, description: 'Analyze those files what have changed or new ones')

        // JIRA Variables

        string(name: 'JIRA_TOKEN_ID', defaultValue:"jira-token" , description: 'Jira token id')

        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')

        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')

        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: true, description: 'Enable Jira reporting')

        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
    }

    agent any
      stages {
        stage('SCANOSS') {

         agent {
            docker {
                image params.SCANOSS_CLI_DOCKER_IMAGE
                args '--entrypoint='
                // Run the container on the node specified at the
                // top-level of the Pipeline, in the same workspace,
                // rather than on a new node entirely:
                reuseNode true
            }
        }
          steps {

                /****** Checkout repository ****/

                script {
                    dir('repository') {
                        git branch: 'main',
                            credentialsId: params.GITHUB_TOKEN_ID,
                            url: 'https://github.com/scanoss/integrations-jenkins'
                    }
                }


                /***** Delta *****/

                script {

                    if (params.ENABLE_DELTA_ANALYSIS == true) {


                        echo 'Delta Scan Analysis enabled'

                        // Parse the JSON payload
                        def payloadJson = readJSON text: env.payload

                        def commits = payloadJson.commits

                        // Define the destination folder
                        def destinationFolder = "${env.WORKSPACE}/delta"

                        // Define a set to store unique file names
                        def uniqueFileNames = new HashSet()

                        // Remove the destination folder if it exists
                        sh "rm -rf ${destinationFolder}"

                        // Create the destination folder if it doesn't exist
                        sh "mkdir -p ${destinationFolder}"


                            // Iterate over each commit
                            commits.each { commit ->


                                // Modified files
                                commit.modified.each { fileName ->
                                    // Trim any leading or trailing whitespaces
                                    fileName = fileName.trim()

                                    uniqueFileNames.add(fileName)
                                }

                                // New files added
                                commit.added.each { fileName ->
                                    // Trim any leading or trailing whitespaces
                                    fileName = fileName.trim()

                                    uniqueFileNames.add(fileName)

                                }
                            }
                        dir('repository'){
                            uniqueFileNames.each { file ->

                                // Construct the source and destination paths
                                        def sourcePath = "${file}"
                                        def destinationPath = "${destinationFolder}"

                                        // Copy the file
                                    sh "cp --parents ${sourcePath} ${destinationPath}"

                            }
                        }
                    }

                }



                /***** Scan *****/

                script {
                     env.SCAN_FOLDER = params.ENABLE_DELTA_ANALYSIS ? 'delta' : 'repository'
                }

                withCredentials([string(credentialsId: params.SCANOSS_API_TOKEN_ID , variable: 'SCANOSS_API_TOKEN')]) {
                    dir("${SCAN_FOLDER}") {
                        script {
                             sh '''
                                 SBOM_IDENTIFY=""
                                 if [ -f $SCANOSS_SBOM_IDENTIFY ]; then SBOM_IDENTIFY="--identify $SCANOSS_SBOM_IDENTIFY" ; fi

                                 SBOM_IGNORE=""
                                 if [ -f $SCANOSS_SBOM_IGNORE ]; then SBOM_IGNORE="--ignore $SCANOSS_SBOM_IGNORE" ; fi

                                 CUSTOM_URL=""
                                 if [ ! -z $SCANOSS_API_URL ]; then CUSTOM_URL="--apiurl $SCANOSS_API_URL"; else CUSTOM_URL="--apiurl https://osskb.org/api/scan/direct" ; fi

                                 CUSTOM_TOKEN=""
                                 if [ ! -z $SCANOSS_API_TOKEN ]; then CUSTOM_TOKEN="--key $SCANOSS_API_TOKEN" ; fi


                                 scanoss-py scan $CUSTOM_URL $CUSTOM_TOKEN $SBOM_IDENTIFY $SBOM_IGNORE --output ../scanoss-results.json .
                            '''

                        }
                    }
                }


                /***** Upload Artifacts *****/

                archiveArtifacts artifacts: 'scanoss-results.json', onlyIfSuccessful: true


                /***** Analyze results *****/

                script {
                        try {

                          //TODO: Remove this when SCANOSS-PY new image be packaged
                          sh 'apt update && apt install jq -y'

                          sh 'echo "component,name,copyleft" > data.csv'
                          sh ''' jq -r 'reduce .[]?[] as $item ({}; select($item.purl) | .[$item.purl[0] + "@" + $item.version] += [$item.licenses[]? | select(.copyleft == "yes") | .name]) | to_entries[] | select(.value | unique | length > 0) | [.key, .key, (.value | unique | length)] | @csv' scanoss-results.json >> data.csv'''

                          env.check_result = sh(script: 'result=$(if [ $(wc -l < data.csv) -gt 1 ]; then echo "1"; else echo "0"; fi); echo $result', returnStdout: true).trim()
                          sh 'echo CHECK RESULT: ${check_result}'

                          if (params.ABORT_ON_POLICY_FAILURE && env.check_result != '0') {
                            currentBuild.result = "FAILURE"
                          }

                        } catch(e) {
                            echo e.getMessage()
                            if (params.ABORT_ON_POLICY_FAILURE) {
                                currentBuild.result = "FAILURE"
                            }
                        }
                }


                /***** Publish report on Jenkins dashboard *****/

                publishReport name: "Scan Results", displayType: "dual", provider: csv(id: "report-summary", pattern: "data.csv")


                /***** Jira issue *****/

                 withCredentials([usernamePassword(credentialsId: params.JIRA_TOKEN_ID ,usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                    script {

                        if ((params.CREATE_JIRA_ISSUE == true) &&  (env.check_result != '0')) {

                            //TODO: Should we move to SCANOSS-PY CLI?
                            sh 'apt update && apt install curl -y'


                            echo "JIRA issue parameter value: ${params.CREATE_JIRA_ISSUE}"


                            def copyLeft = sh(script: "tail -n +2 data.csv | cut -d',' -f1", returnStdout: true)

                            copyLeft = copyLeft +  "\n${BUILD_URL}"

                            def JSON_PAYLOAD =  [
                                fields : [
                                    project : [
                                        key: params.JIRA_PROJECT_KEY
                                    ],
                                    summary : 'Components with Copyleft licenses found',
                                    description: copyLeft,
                                    issuetype: [
                                        name: 'Bug'
                                    ]
                                ]
                            ]

                            def jsonString = groovy.json.JsonOutput.toJson(JSON_PAYLOAD)

                            createJiraIssue(PASSWORD, USERNAME, params.JIRA_URL, jsonString)
                        }
                    }
                }
            }
        }
    }
}


def createJiraIssue(jiraToken, jiraUsername, jiraAPIEndpoint, payload) {
    env.TOKEN = jiraToken
    env.USER = jiraUsername
    env.JIRA_ENDPOINT_URL = jiraAPIEndpoint + '/rest/api/2/issue/'
    env.PAYLOAD = payload

    try {
        def command = """
            curl -u '${USER}:${TOKEN}' -X POST --data '${PAYLOAD}' -H 'Content-Type: application/json' '${JIRA_ENDPOINT_URL}'
        """

        def response = sh(script: command, returnStdout: true).trim()
        echo "Response: ${response}"

    } catch (Exception e) {
        echo e
    }
}
