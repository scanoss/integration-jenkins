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
            when {
                expression {
                     def payload = readJSON text: "${env.payload}"

                     return payload.pull_request !=  null && payload.pull_request.base.ref == 'main' && payload.action == 'opened'
                }
            }

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
                script {
                    
                    /***** File names *****/
                    env.SCANOSS_RESULTS_FILE_NAME = "scanoss-results.json"
                    env.SCANOSS_LICENSE_FILE_NAME = "scanoss_license_data.csv"
                    env.SCANOSS_COPYLEFT_FILE_NAME = "copyleft.md"
                   

                    
                    /****** Create Resources folder ******/
                    env.SCANOSS_RESOURCE_PATH = "scan_reports/scanoss_${currentBuild.number}"
                    sh "mkdir -p ${env.SCANOSS_RESOURCE_PATH}"

                    /***** Resources Paths *****/
                    env.LICENSE_RESOURCE_PATH = "${env.SCANOSS_RESOURCE_PATH}/${env.SCANOSS_LICENSE_FILE_NAME}"
                    env.SCANOSS_RESULTS_PATH = "${env.SCANOSS_RESOURCE_PATH}/${SCANOSS_RESULTS_FILE_NAME}"
                    env.COPYLEFT_RESOURCE_PATH = "${env.SCANOSS_RESOURCE_PATH}/${env.SCANOSS_COPYLEFT_FILE_NAME}"




                    /****** Get Repository name and repo URL from payload ******/
                    def payloadJson = readJSON text: env.payload
                    env.REPOSITORY_NAME = payloadJson.pull_request.base.repo.name
                    env.REPOSITORY_URL = payloadJson.pull_request.base.repo.html_url


                    /****** Checkout repository ******/
                    dir('repository') {
                        git branch: 'main',
                            credentialsId: params.GITHUB_TOKEN_ID,
                            url: 'https://github.com/scanoss/integrations-jenkins'
                    }


                    /***** Delta *****/
                    deltaScan()


                    /***** Scan *****/
                    env.SCAN_FOLDER = params.ENABLE_DELTA_ANALYSIS ? 'delta' : 'repository'
                    scan()

                    /***** Upload Artifacts *****/
                    uploadArtifacts()


                    /**** Analyze results for copyleft ****/
                    copyleft()


                    /***** Publish report on Jenkins dashboard *****/
                    publishReport()


                    /***** Jira issue *****/
                    withCredentials([usernamePassword(credentialsId: params.JIRA_TOKEN_ID ,usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                        script {

                            if ((params.CREATE_JIRA_ISSUE == true) &&  (env.check_result != '0')) {


                                echo "JIRA issue parameter value: ${params.CREATE_JIRA_ISSUE}"


                               def copyLeft = sh(script: "cat ${COPYLEFT_RESOURCE_PATH}", returnStdout: true)

                               copyLeft = copyLeft +  "\nMore details can be found: ${BUILD_URL}\nSource repository: ${env.REPOSITORY_URL}"

                                def JSON_PAYLOAD =  [
                                    fields : [
                                        project : [
                                            key: params.JIRA_PROJECT_KEY
                                        ],
                                        summary : "Components with Copyleft licenses found at ${env.REPOSITORY_NAME}",
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
}


def publishReport() {
     publishReport name: "Scan Results", displayType: "dual", provider: csv(id: "report-summary", pattern: "${env.LICENSE_RESOURCE_PATH}")
}

def copyleft() {
    try {

         def check_result = sh (returnStdout: true, script: '''
            echo 'component,name,copyleft' > $LICENSE_RESOURCE_PATH

            jq -r 'reduce .[]?[] as \$item ({}; select(\$item.purl) | .[\$item.purl[0] + \"@\" + \$item.version] += [\$item.licenses[]? | select(.copyleft == \"yes\") | .name]) | to_entries[] | select(.value | unique | length > 0) | [.key, .key, (.value | unique | length)] | @csv' $SCANOSS_RESULTS_PATH >> $LICENSE_RESOURCE_PATH
            
            
            printf '|| Component || Purl || Version || Licenses||\n' > $COPYLEFT_RESOURCE_PATH           

            jq -r '[.[]?[] | select(.licenses) | select(.licenses[] | .copyleft? == \"yes\") | {component: .component, version: .version, purl: .purl[], licenses: (.licenses | map(.name) | join(\",\"))}] | unique_by(.purl) | sort_by(.component) | to_entries[] | "|\\(.value.component)|\\(.value.purl)|\\(.value.version)|\\(.value.licenses)|"'  $SCANOSS_RESULTS_PATH >> $COPYLEFT_RESOURCE_PATH 
           

            check_result=$(if [ $(wc -l < $LICENSE_RESOURCE_PATH) -gt 1 ]; then echo "1"; else echo "0"; fi);
            echo \$check_result
         ''')

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


 def uploadArtifacts() {
  def scanossResultsPath = "${env.SCANOSS_RESULTS_PATH}"
  archiveArtifacts artifacts: scanossResultsPath, onlyIfSuccessful: true
}


def scan() {
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


                 scanoss-py scan $CUSTOM_URL $CUSTOM_TOKEN $SBOM_IDENTIFY $SBOM_IGNORE --output ../$SCANOSS_RESULTS_FILE_NAME .

                 cp ../$SCANOSS_RESULTS_FILE_NAME $WORKSPACE/$SCANOSS_RESOURCE_PATH/$SCANOSS_RESULTS_FILE_NAME

            '''

        }
    }
  }
}

def deltaScan() {
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

