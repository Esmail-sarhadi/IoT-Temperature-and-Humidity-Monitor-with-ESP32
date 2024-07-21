#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>   
#include <SPIFFS.h>     
#include <ArduinoJson.h>  
     
// WiFi settings   
const char* ssid = "charon";     
const char* password = "12121212"; 
    
// DHT settings  
#define DHTPIN 4 // Pin connected to the DHT  
#define DHTTYPE DHT21 
DHT dht(DHTPIN, DHTTYPE); 

// GPIO settings for the lamp
#define LAMP_PIN 2 
     
// Create a web server
WebServer server(80);
   
void setup() {
  // Start serial communication for displaying information
  Serial.begin(115200);

  // Format SPIFFS (only for the first time, then comment this line)
  formatSPIFFS();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
 
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Start DHT
  dht.begin();

  // Set GPIO for the lamp
  pinMode(LAMP_PIN, OUTPUT);
  digitalWrite(LAMP_PIN, LOW); // Turn off the lamp initially

  // Set route for the main page
  server.on("/", handle_OnConnect);
  server.on("/toggle-lamp", handle_ToggleLamp);
  server.on("/read-data", handle_ReadData);
  server.on("/download", handle_Download);
  server.onNotFound(handle_NotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handle_OnConnect() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>Temperature and Humidity Information</title>";
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f2f2f2; color: #333; text-align: center; padding: 50px; }";
  html += "h1 { color: #4CAF50; }";
  html += ".data { font-size: 1.5em; margin: 20px 0; opacity: 0; animation: fadeIn 2s forwards; }";
  html += ".container { background-color: #fff; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); display: inline-block; padding: 20px 40px; opacity: 0; animation: fadeIn 1s forwards; }";
  html += ".icon { font-size: 2em; margin-right: 10px; }";
  html += ".btn { padding: 10px 20px; font-size: 1em; margin-top: 20px; cursor: pointer; border: none; border-radius: 5px; background-color: #4CAF50; color: white; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += "@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }";
  html += "</style></head>";
  html += "<body><div class='container'>";
  html += "<h1><i class='fas fa-microchip icon'></i>Temperature and Humidity Information</h1>";
  html += "<p class='data' id='temp'><i class='fas fa-thermometer-half icon'></i>Temperature: -- °C</p>";
  html += "<p class='data' id='humidity'><i class='fas fa-tint icon'></i>Humidity: -- %</p>";
  html += "<p class='data'><i class='fas fa-wifi icon'></i>WiFi Status: Connected</p>";
  html += "<p class='data'><i class='fas fa-server icon'></i>Web Server Running</p>";
  html += "<button class='btn' onclick='toggleLamp()'><i class='fas fa-lightbulb icon'></i>Toggle Lamp</button>";
  html += "<button class='btn' onclick='downloadFile()'><i class='fas fa-download icon'></i>Download Data</button>";
  html += "</div>";
  html += "<script>";
  html += "function toggleLamp() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/toggle-lamp', true);";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      alert('Lamp state toggled.');";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function downloadFile() {";
  html += "  window.location.href = '/download';";
  html += "}";
  html += "function updateColors(temp, humidity) {";
  html += "  var tempElement = document.getElementById('temp');";
  html += "  var humidityElement = document.getElementById('humidity');";
  html += "  if (temp > 24) { tempElement.style.color = 'red'; }";
  html += "  else if (temp < 21) { tempElement.style.color = 'blue'; }";
  html += "  else { tempElement.style.color = 'green'; }";
  html += "  if (humidity > 50) { humidityElement.style.color = 'red'; }";
  html += "  else if (humidity < 40) { humidityElement.style.color = 'blue'; }";
  html += "  else { humidityElement.style.color = 'green'; }";
  html += "}";
  html += "setInterval(function() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/read-data', true);";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      var data = JSON.parse(xhr.responseText);";
  html += "      document.getElementById('temp').innerHTML = '<i class=\"fas fa-thermometer-half icon\"></i>Temperature: ' + data.temperature + ' °C';";
  html += "      document.getElementById('humidity').innerHTML = '<i class=\"fas fa-tint icon\"></i>Humidity: ' + data.humidity + ' %';";
  html += "      updateColors(data.temperature, data.humidity);";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}, 500);"; // Update every 10 seconds
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handle_ToggleLamp() {
  int lampState = digitalRead(LAMP_PIN);
  digitalWrite(LAMP_PIN, !lampState); // Toggle lamp state
  server.send(200, "text/plain", "Lamp state toggled.");
}

void handle_ReadData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Save data to a file
  File file = SPIFFS.open("/data.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  file.printf("Temperature: %.2f °C, Humidity: %.2f %%\n", temperature, humidity);
  file.close();

  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity);
  json += "}";
  server.send(200, "application/json", json);
}

void handle_Download() {
  File file = SPIFFS.open("/data.txt", FILE_READ);
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(file, "text/plain");
  file.close();
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void formatSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  Serial.println("SPIFFS formatted successfully");
}
