# LoRa Web Monitor

This project is a web-based monitoring system for a LoRa (Long Range) communication setup. It allows users to monitor the status of the LoRa receiver, view logs of received data, and manage WiFi connections through a user-friendly web interface.

## Features

- **Real-time Monitoring**: Displays the current status of the LoRa receiver and logs received data.
- **WiFi Management**: Provides a web interface for connecting to and managing WiFi networks.
- **Data Logging**: Logs all received data from the LoRa receiver for later review.
- **Responsive Design**: The web interface is designed to be responsive and accessible on various devices.

## Project Structure

- `src/main.cpp`: Entry point of the application, initializes the web server and LoRa functionalities.
- `src/web`: Contains the web server implementation and related classes.
- `src/data`: Contains classes for data logging and LoRa data reception.
- `src/utils`: Utility functions and constants used throughout the application.
- `src/web_assets`: HTML, CSS, and JavaScript files for the web interface.
- `data`: Contains configuration files and favicon for the web application.
- `lib`: Documentation for any libraries used in the project.

## Setup Instructions

1. **Clone the Repository**: 
   ```
   git clone <repository-url>
   cd lora-web-monitor
   ```

2. **Install Dependencies**: 
   Use PlatformIO to install the necessary libraries and dependencies.

3. **Configure WiFi Settings**: 
   Edit the `data/config.json` file to include your WiFi credentials.

4. **Build and Upload**: 
   Use PlatformIO to build the project and upload it to your compatible board.

5. **Access the Web Interface**: 
   Open a web browser and navigate to the IP address of your device to access the monitoring dashboard.

## Usage

- **Monitoring Dashboard**: View the current status of the LoRa receiver and logs of received data.
- **WiFi Configuration**: Use the WiFi management page to connect to different networks.
- **Data Logs**: Access the logs page to review all received data from the LoRa receiver.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.