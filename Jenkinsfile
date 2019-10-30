pipeline {
    agent {
        label 'windows && a102'
    }
    stages {
        stage('Test') {
            steps {
                bat 'pytest tests --junitxml=report.xml -s'
            }
        }
    }
    post {
        always {
            junit 'report.xml'
            archiveArtifacts artifacts: '*.png', onlyIfSuccessful: true
            cleanWs()
        }
    }
}