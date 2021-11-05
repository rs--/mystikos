/* A Jenkins pipeline that will handle code coverage and nightly tests
*  These are the original pipelines:
*  https://github.com/deislabs/mystikos/blob/main/.azure_pipelines/ci-pipeline-code-coverage-nightly.yml
*/

pipeline {
    agent {
        label 'Jenkins-Shared-DC2'
    }
    options {
        timeout(time: 300, unit: 'MINUTES')
    }
    parameters {
        string(name: "REPOSITORY", defaultValue: "deislabs")
        string(name: "BRANCH", defaultValue: "main", description: "Branch to build")
        choice(name: "REGION", choices:['useast', 'canadacentral'], description: "Azure region for solutions test")
    }
    stages {
        stage('Run Tests') {
            parallel {
                stage("Run Unit Tests") {
                    steps {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        build job: "Standalone-Pipelines/Unit-Tests-Pipeline",
                        parameters: [
                            string(name: "REPOSITORY", value: REPOSITORY),
                            string(name: "BRANCH", value: BRANCH),
                            string(name: "TEST_CONFIG", value: "None"),
                            string(name: "COMMIT_SYNC", value: GIT_COMMIT)
                        ]
                    }}
                }
                stage("Run Solutions Tests") {
                    steps {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        build job: "Standalone-Pipelines/Solutions-PR-Tests-Pipeline",
                        parameters: [
                            string(name: "REPOSITORY", value: REPOSITORY),
                            string(name: "BRANCH", value: BRANCH),
                            string(name: "REGION", value: REGION),
                            string(name: "COMMIT_SYNC", value: GIT_COMMIT)
                        ]
                    }}
                }
            }
        }
        stage('Cleanup') {
            steps {
                cleanWs()
            }
        }
    }
}
