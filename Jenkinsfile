pipeline {
    agent {
        docker { image 'devops/fedora-pio-toolchain:latest' }
    }

    stages {
        stage('Versions') {
            steps {
                sh 'echo $PATH'
                // Print versions of tools
                sh 'python3 --version'
                sh 'pio --version'
                sh 'pio pkg list --global'
            }
        }

        stage('Build (PlatformIO)') {
            steps {
                sh 'pio run -v'
            }
        }

        // stage('Export output') {
        //     steps {
                
        //     }
        // }
    }
    post {
        // always {
        //     dir("$WORKSPACE_BUILD_DIR") {
        //         deleteDir()
        //     }
        // }
        success {
            archiveArtifacts artifacts: ".pio/*/*.hex", fingerprint: true
            archiveArtifacts artifacts: ".pio/*/*.elf", fingerprint: true
        }
        // failure {
        // }
        // unstable {
        // }
        // changed {
        // }
    }
}
