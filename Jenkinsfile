pipeline {
    agent {
        label 'windows && a102'
    }
    stages {
        stage('Test') {
            steps {
                bat 'pytest tests --junitxml=report.xml -s -x'
            }
        }
    }
    post {
        always {
            junit 'report.xml'
        }
    }
}