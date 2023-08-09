pipeline {
    agent {
        docker {
            image 'devops/fedora-pio-toolchain:latest'
            args '--entrypoint=""'
        }
    }

    parameters {
        choice(
            name: 'DISASSEMBLY',
            choices: ['no', "yes"],
            description: 'Tells if the disassembly should be generated'
        )
        string(
            name: 'ENVIROMENT',
            defaultValue: "",
            description: 'Application (environment) to build, leave empty to build everything'
        )
    }

    stages {
        stage('Git checkout submodules') {
            steps {
                sh 'git submodule update --init --recursive'
            }
        }

        stage('Versions') {
            steps {
                sh 'echo $PATH'
                // Print versions of tools
                sh 'python3 --version'
                sh 'pio --version'
                sh 'pio pkg list'
            }
        }

        stage('Build single application (PlatformIO)') {
            when {
                expression { params.ENVIROMENT != "" }
            }
            steps {
                sh "pio run -v -e ${params.ENVIROMENT}"
            }
        }

        stage('Build all (PlatformIO)') {
            when {
                expression { params.ENVIROMENT == "" }
            }
            steps {
                sh 'pio run -v'
            }
        }

        stage('Generate disassembly') {
            when {
                expression { params.DISASSEMBLY == "yes" }
            }
            steps {
                sh "python3 lib/AVRTOS/scripts/piodis.py"
            }
        }
        stage('Publish') {
            steps {
                archiveArtifacts artifacts: ".pio/*/*/*.hex", fingerprint: true
                archiveArtifacts artifacts: ".pio/*/*/*.elf", fingerprint: true
                archiveArtifacts artifacts: ".pio/*/*/target-infos.txt", fingerprint: true
    
                archiveArtifacts artifacts: ".pio/*/*/*.txt", fingerprint: true, allowEmptyArchive: true
                archiveArtifacts artifacts: ".pio/*/*/*.s", fingerprint: true, allowEmptyArchive: true
            }
        }
    }
    post {
        always {
            dir("${env.WORKSPACE}") {
                deleteDir()
            }
        }
    }
}
