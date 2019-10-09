pipeline {
    agent {
        label 'windows && a102'
    }
    stages {
        stage('Test') {
            steps {
                bat 'pytest --junitxml=report.xml'
            }
        }
    }
    post {
        always {
            junit 'report.xml'
        }
    }
}