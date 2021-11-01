/* A Jenkins pipeline that will handle code coverage and nightly tests
*  These are the original pipelines:
*  https://github.com/deislabs/mystikos/blob/main/.azure_pipelines/ci-pipeline-code-coverage-nightly.yml
*/

pipeline {
    agent {
        label 'ACC-1804-DC8-build-machine'
    }
    options {
        timeout(time: 300, unit: 'MINUTES')
    }
    parameters {
        string(name: "REPOSITORY", defaultValue: "deislabs")
        string(name: "BRANCH", defaultValue: "main", description: "Branch to build")
        choice(name: "TEST_CONFIG", choices:['Nightly', 'Code Coverage'], description: "Test configuration to execute")
        choice(name: "REGION", choices:['useast', 'canadacentral'], description: "Azure region for SQL test")
    }
    environment {
        MYST_SCRIPTS =      "${WORKSPACE}/scripts"
        JENKINS_SCRIPTS =   "${WORKSPACE}/.jenkins/scripts"
        MYST_NIGHTLY_TEST = 1
        MYST_ENABLE_GCOV =  1
        BUILD_RESOURCES =   "build-resources-${GIT_COMMIT[0..7]}"
        BUILD_USER = sh(
            returnStdout: true,
            script: 'echo \${USER}'
        )
    }
    stages {
        stage("Initialize Workspace") {
            steps {
                sh """
                   ${JENKINS_SCRIPTS}/global/clean-temp.sh
                   """
                azureDownload(
                    downloadType: 'container',
                    containerName: 'mystikos-build-resources',
                    includeFilesPattern: "${BUILD_RESOURCES}",
                    storageCredentialId: 'mystikosreleaseblobcontainer'
                )
            }
        }
        stage('Init Config') {
            when {
                not { expression { return fileExists("${BUILD_RESOURCES}") }}
            }
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
            when {
                not { expression { return fileExists("${BUILD_RESOURCES}") }}
            }
            steps {
                sh """
                   ${JENKINS_SCRIPTS}/global/make-world.sh
                   tar -zcf ${BUILD_RESOURCES} build
                   """
            }
        }
        stage('Upload build resources') {
            steps {
                sh """
                   echo "Uploading build resources: ${BUILD_RESOURCES}"
                   """
                // Lifecycle management > build-resources-retention
                // Build resources are automatically deleted 2 days
                // the last modification
                azureUpload(
                    containerName: 'mystikos-build-resources',
                    storageType: 'container',
                    uploadZips: true,
                    filesPath: "${BUILD_RESOURCES}",
                    storageCredentialId: 'mystikosreleaseblobcontainer'
                )
            }
        }
        stage('Run Tests') {
            parallel {
                stage("Run Unit Tests") {
                    steps {
                        catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                            sh """
                               ${JENKINS_SCRIPTS}/global/make-tests.sh
                               """
                        }
                    }
                }
                stage("Run SQL Tests") {
                    steps {
                        withCredentials([string(credentialsId: 'Jenkins-ServicePrincipal-ID', variable: 'SERVICE_PRINCIPAL_ID'),
                                         string(credentialsId: 'Jenkins-ServicePrincipal-Password', variable: 'SERVICE_PRINCIPAL_PASSWORD'),
                                         string(credentialsId: 'ACC-Prod-Tenant-ID', variable: 'TENANT_ID'),
                                         string(credentialsId: 'ACC-Prod-Subscription-ID', variable: 'AZURE_SUBSCRIPTION_ID'),
                                         string(credentialsId: 'oe-jenkins-dev-rg', variable: 'JENKINS_RESOURCE_GROUP'),
                                         string(credentialsId: 'mystikos-managed-identity', variable: "MYSTIKOS_MANAGED_ID"),
                                         string(credentialsId: "mystikos-sql-db-name-${REGION}", variable: 'DB_NAME'),
                                         string(credentialsId: "mystikos-sql-db-server-name-${REGION}", variable: 'DB_SERVER_NAME'),
                                         string(credentialsId: "mystikos-maa-url-${REGION}", variable: 'MAA_URL'),
                                         string(credentialsId: 'mystikos-managed-identity-objectid', variable: 'DB_USERID'),
                                         string(credentialsId: 'mystikos-mhsm-client-secret', variable: 'CLIENT_SECRET'),
                                         string(credentialsId: 'mystikos-mhsm-client-id', variable: 'CLIENT_ID'),
                                         string(credentialsId: 'mystikos-mhsm-app-id', variable: 'APP_ID'),
                                         string(credentialsId: 'mystikos-mhsm-aad-url', variable: 'MHSM_AAD_URL'),
                                         string(credentialsId: 'mystikos-mhsm-ssr-pkey', variable: 'SSR_PKEY')
                        ]) {
                            sh """
                               ${JENKINS_SCRIPTS}/solutions/init-config.sh
                               ${JENKINS_SCRIPTS}/global/wait-dpkg.sh
                               ${JENKINS_SCRIPTS}/solutions/azure-config.sh

                               echo "Running in ${REGION}"
                               make tests -C ${WORKSPACE}/solutions
                               """
                        }
                    }
                }
                stage("Run DotNet Tests") {
                    steps {
                        build job: "Helper-Pipelines/DotNet-Test-Pipeline",
                        parameters: [
                            string(name: "REPOSITORY", value: REPOSITORY),
                            string(name: "BRANCH", value: BRANCH),
                            string(name: "BUILD_RESOURCES", value: BUILD_RESOURCES)
                        ]
                    }
                }
                stage("Run Azure SDK Tests") {
                    steps {
                        build job: "Helper-Pipelines/Azure-SDK-Test-Pipeline",
                        parameters: [
                            string(name: "REPOSITORY", value: REPOSITORY),
                            string(name: "BRANCH", value: BRANCH),
                            string(name: "BUILD_RESOURCES", value: BUILD_RESOURCES)
                        ]
                    }
                }
            }
        }
        stage('Measure and Report Code Coverage') {
            when {
                expression { params.TEST_CONFIG == 'Code Coverage' }
            }
            steps {
                script {
                    LCOV_DIR="mystikos-cc-${env.GIT_COMMIT}"
                }

                sh """
                   ${MYST_SCRIPTS}/myst_cc

                   mkdir ${LCOV_DIR}
                   mv lcov* ${LCOV_DIR}
                   tar -zcvf ${LCOV_DIR}.tar.gz ${LCOV_DIR}
                   """

                azureUpload(
                    containerName: 'mystikos-code-coverage',
                    storageType: 'container',
                    uploadZips: true,
                    filesPath: "${LCOV_DIR}.tar.gz",
                    storageCredentialId: 'mystikosreleaseblobcontainer'
                )
            }
        }
        stage('Cleanup') {
            steps {
                cleanWs()
            }
        }
    }
}
