pipeline {
    agent {
        label 'ACC-1804-DC2'
    }
    options {
        timeout(time: 30, unit: 'MINUTES')
    }
    parameters {
        string(name: "REPOSITORY", defaultValue: "deislabs")
        string(name: "BRANCH", defaultValue: "main", description: "Branch to build")
        string(name: "COMMIT_ID", description: "Short commit ID used to archive build resoures")
    }
    environment {
        MYST_SCRIPTS =      "${WORKSPACE}/scripts"
        JENKINS_SCRIPTS =   "${WORKSPACE}/.jenkins/scripts"
        MYST_NIGHTLY_TEST = 1
        MYST_ENABLE_GCOV =  1
        LCOV_PREFIX =       "lcov-${COMMIT_ID}"
        LCOV_RESOURCES =    "${LCOV_PREFIX}.tar.gz"
        BUILD_USER = sh(
            returnStdout: true,
            script: 'echo \${USER}'
        )
    }
    stages {
        stage('Measure code coverage') {
            steps {
                azureDownload(
                    downloadType: 'container',
                    containerName: 'mystikos-code-coverage',
                    includeFilesPattern: "${LCOV_PREFIX}-*",
                    storageCredentialId: 'mystikosreleaseblobcontainer'
                )

                sh """
                   ls -l

                   # Initialize dependencies repo
                   ${JENKINS_SCRIPTS}/global/init-config.sh

                   # Install global dependencies
                   ${JENKINS_SCRIPTS}/global/wait-dpkg.sh
                   ${JENKINS_SCRIPTS}/global/init-install.sh

                   ${JENKINS_SCRIPTS}/global/wait-dpkg.sh
                   ${JENKINS_SCRIPTS}/code-coverage/init-install.sh

                   for archive in ${LCOV_PREFIX}-*; do tar xzf \$archive; done

                   lcov -a lcov/${LCOV_PREFIX}-dotnet.info -a lcov/${LCOV_PREFIX}-sdk.info -a lcov/${LCOV_PREFIX}-solutions.info -a lcov/${LCOV_PREFIX}-unit.info -o lcov/lcov.info
                   lcov --list lcov/lcov.info | tee -a code-coverage-report

                   cp lcov/lcov.info ${LCOV_PREFIX}.info
                   mv lcov ${LCOV_PREFIX}
                   tar -zcvf ${LCOV_RESOURCES} ${LCOV_PREFIX}
                   """

                azureUpload(
                    containerName: 'mystikos-code-coverage',
                    storageType: 'container',
                    uploadZips: true,
                    filesPath: "${LCOV_PREFIX}.info",
                    storageCredentialId: 'mystikosreleaseblobcontainer'
                )
                azureUpload(
                    containerName: 'mystikos-code-coverage',
                    storageType: 'container',
                    uploadZips: true,
                    filesPath: "${LCOV_RESOURCES}",
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
