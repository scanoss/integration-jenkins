# Contributing
Thank you for considering contributing to SCANOSS. It's people like you that make SCANOSS Jenkins Integration better. Feel welcome and read the following sections in order to know how to get involved, ask questions and more importantly how to work on something.

The SCANOSS Jenkins Integration is an open source project, and we love to receive contributions from our community. There are many ways to contribute, from writing tutorials or blog posts, improving the documentation, submitting bug reports and feature requests, or writing code. A welcome addition to the project is an integration with a new source code repository.


### Submitting bugs

If you are submitting a bug, please tell us:

- Pipeline you are using
- How to reproduce the bug.

### Pull requests

Want to submit a pull request? Great! But please follow some basic rules:

- Write a brief description that help us understand what you are trying to accomplish: what the change does, link to any relevant issue
- If you are changing a source file please make sure that you only include in the changeset the lines changed by you (beware of your editor reformatting the file)


## Development Setup

### Prerequisites
- Docker Desktop installed and running
- Terminal access
- Port 8080 and 50000 available on your machine

### Installation


#### Linux Installation

1. Install Docker using your distribution's package manager:

   For Ubuntu/Debian:
   ```bash
   sudo apt update
   sudo apt install docker.io
   ```

   For Fedora:
   ```bash
   sudo dnf install docker
   ```

2. Start and enable Docker service:
   ```bash
   sudo systemctl start docker
   sudo systemctl enable docker
   ```

3. Add your user to the docker group:
   ```bash
   sudo usermod -aG docker $USER
   ```
   Note: Log out and back in for this to take effect.

4. Create a dedicated Jenkins network:
   ```bash
   docker network create jenkins
   ```

5. Pull the latest Jenkins LTS image:
   ```bash
   docker pull jenkins/jenkins:lts
   ```

6. Start the Jenkins container:
   ```bash
   docker run -d \
     -p 8080:8080 \
     -p 50000:50000 \
     --name jenkins-container \
     --network jenkins \
     jenkins/jenkins:lts
   ```

#### macOS Installation

1. Install Docker Desktop for macOS:
   ```bash
   brew install --cask docker
   ```

2. Start Docker Desktop and verify installation:
   ```bash
   docker --version
   ```

3. Create a dedicated Jenkins network:
   ```bash
   docker network create jenkins
   ```

4. Start the Jenkins container:
   ```bash
    docker run -d --name jenkins-container -p 8080:8080 -p 50000:50000 -v /var/run/docker.sock:/var/run/docker.sock --group-add $(stat -f '%g' /var/run/docker.sock) jenkins/jenkins:lts
   ```
5. Configure Docker inside Jenkins container:
    ```bash
    # Access the container as root
    docker exec -u root -it jenkins-container bash
    
    # Install Docker inside the container
    apt-get update && apt-get install -y docker.io
    
    # Create docker group and add jenkins user
    groupadd -f docker
    usermod -aG docker jenkins
    
    # Fix Docker socket permissions
    chown root:docker /var/run/docker.sock
    chmod 666 /var/run/docker.sock
    ```

### Post-Installation Steps
1. Access Jenkins at `http://localhost:8080`
2. Retrieve the initial admin password:
   ```bash
   docker exec jenkins-container cat /var/jenkins_home/secrets/initialAdminPassword
   ```
