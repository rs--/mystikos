pipeline {
    agent {
        label 'ACC-1804-DC4'
    }
    options {
        timeout(time: 300, unit: 'MINUTES')
    }
    parameters {
        string(name: "REPOSITORY", defaultValue: "deislabs")
        string(name: "BRANCH", defaultValue: "main", description: "Branch to build")
        choice(name: "REGION", choices:['useast', 'canadacentral'], description: "Azure region for solution tests")
        string(name: "COMMIT_SYNC", description: "optional - used to sync outputs of parallel jobs")
    }
    environment {
        MYST_SCRIPTS =      "${WORKSPACE}/scripts"
        JENKINS_SCRIPTS =   "${WORKSPACE}/.jenkins/scripts"
        MYST_SKIP_PR_TEST = 1
        BUILD_USER = sh(
            returnStdout: true,
            script: 'echo \${USER}'
        )
    }
    stages {
        stage("Cleanup files") {
            steps {
                sh """
                   ${JENKINS_SCRIPTS}/global/clean-temp.sh
                   """
            }
        }
        stage('Verify commit sync') {
            when { allOf {
                expression { params.COMMIT_SYNC != "" }
                expression { params.COMMIT_SYNC != GIT_COMMIT }
            }}
            steps {
                script {
                    currentBuild.result = 'ABORTED'
                    error("Aborting build: mismatched commit - commit($GIT_COMMIT), expected(${COMMIT_SYNC})")
                }
            }
        }
        stage('Init Config') {
            steps {
                checkout([$class: 'GitSCM',
                    branches: [[name: BRANCH]],
                    extensions: [],
                    userRemoteConfigs: [[url: 'https://github.com/${REPOSITORY}/mystikos']]])
                sh """
                   # Initialize dependencies repo
                   ${JENKINS_SCRIPTS}/global/init-config.sh

                   # Install global dependencies
                   ${JENKINS_SCRIPTS}/global/wait-dpkg.sh
                   ${JENKINS_SCRIPTS}/global/init-install.sh
                   """
            }
        }
        stage('Build repo source') {
            steps {
                sh """
                   ${JENKINS_SCRIPTS}/global/make-world.sh
                   """
            }
        }
        stage('Setup Solutions Access') {
            steps {
                withCredentials([string(credentialsId: 'Jenkins-ServicePrincipal-ID', variable: 'SERVICE_PRINCIPAL_ID'),
                                 string(credentialsId: 'Jenkins-ServicePrincipal-Password', variable: 'SERVICE_PRINCIPAL_PASSWORD'),
                                 string(credentialsId: 'ACC-Prod-Tenant-ID', variable: 'TENANT_ID'),
                                 string(credentialsId: 'ACC-Prod-Subscription-ID', variable: 'AZURE_SUBSCRIPTION_ID'),
                                 string(credentialsId: 'oe-jenkins-dev-rg', variable: 'JENKINS_RESOURCE_GROUP'),
                                 string(credentialsId: 'mystikos-managed-identity', variable: "MYSTIKOS_MANAGED_ID")]) {
                    sh """
                       ${JENKINS_SCRIPTS}/solutions/init-config.sh
                       ${JENKINS_SCRIPTS}/global/wait-dpkg.sh
                       ${JENKINS_SCRIPTS}/solutions/azure-config.sh
                       """
                }
            }
        }
        stage('Run Solutions Tests - (PR subset)') {
            steps {
                catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                    withCredentials([string(credentialsId: "mystikos-sql-db-name-${REGION}", variable: 'DB_NAME'),
                                     string(credentialsId: "mystikos-sql-db-server-name-${REGION}", variable: 'DB_SERVER_NAME'),
                                     string(credentialsId: "mystikos-maa-url-${REGION}", variable: 'MAA_URL'),
                                     string(credentialsId: 'mystikos-managed-identity-objectid', variable: 'DB_USERID')
                    ]) {
                        sh """
                           echo "Running in ${REGION}"
                           make tests -C ${WORKSPACE}/solutions
                           """
                    }
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
