#include "httpsrv.h"
#include "ESPNtpClient.h"

void HTTPSrv::setup(Data *datastore){
  server = new(ESP8266WebServer);
  data = datastore;
}

void HTTPSrv::begin(){
  server->on("/", std::bind(&HTTPSrv::handleRoot, this));
  server->on("/history", std::bind(&HTTPSrv::handleHistory, this));

  server->on("/config", std::bind(&HTTPSrv::handleConfig, this));
  server->on("/saveconfig", std::bind(&HTTPSrv::handleSave, this));
  server->on("/upload", HTTP_GET, std::bind(&HTTPSrv::handleGetUpload, this));
  server->on("/upload", HTTP_POST, std::bind(&HTTPSrv::handle200, this), std::bind(&HTTPSrv::handleFileUpload, this));
  server->onNotFound(std::bind(&HTTPSrv::handleNotFound, this));
  server->begin();
}

void HTTPSrv::handle() {
  server->handleClient();
}

String HTTPSrv::bootstrapColor(int Color){
  switch (Color)
  {
  case BS_GREEN:
    return "class=\"table-success\"";
    break;
  case BS_YELLOW:
    return "class=\"table-warning\"";
    break;
  case BS_RED:
    return "class=\"table-danger\"";
    break;
  case BS_GREY:
    return "class=\"table-secondary\"";
    break;
  case BS_BLUE:
    return "class=\"table-primary\"";
    break;
  default:
    return "";
    break;
  }
}

String HTTPSrv::int2RowColor(int value, int low, int warn) {
  if (value >= warn){
    return bootstrapColor(BS_GREEN);
  } else if (value >= low) {
    return bootstrapColor(BS_YELLOW);
  } else {
    return bootstrapColor(BS_RED);
  }
}

String HTTPSrv::float2RowColor(float value, float warn, float error) {
  if (value < warn){
    return bootstrapColor(BS_GREEN);
  } else if (value > error) {
    return bootstrapColor(BS_RED);
  } else {
    return bootstrapColor(BS_YELLOW);
  }
}

String HTTPSrv::bool2RowColor(boolean value) {
  if (value) {
    return bootstrapColor(BS_GREEN);
  }
  else
  {
    return bootstrapColor(BS_RED);
  }
}

String HTTPSrv::HTML_Header(int page, bool Bootstrap, bool Chartjs) {
  String message;
  String Title = "Fioul Tank Level";

  message  = "<!DOCTYPE html><html lang=\"en\">";
  message += "<head><meta charset=UTF-8><title>" + Title + "</title>";

  if (Bootstrap) {
  message += "<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta1/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-giJF6kkoqNQ00vy+HMDP7azOuL0xtbfIcaT9wjKHr8RbDVddVHyTfAAsrekwKmP1\" crossorigin=\"anonymous\">";
  message += "<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.0.0-beta1/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-ygbV9kiqUc6oa4msXn9868pTtWMgiQaeYH7/t7LECLbyPA2x65Kgf80OJFdroafW\" crossorigin=\"anonymous\"></script>";
  }
  if (Chartjs) {
    message += "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@2.9.4/dist/Chart.bundle.min.js\" integrity=\"sha256-eA+ych7t31OjiXs3fYU0iWjn9HvXMiCLmunP2Gpghok=\" crossorigin=\"anonymous\"></script>";
  }

  message += "</head><body>";
  message += "<div class=\"p-3 mb-2 bg-primary text-white\"><h1>" + Title + "</h1></div>";

  //message += "<nav class=\"nav nav-pills\">";
  message += "<nav class=\"nav nav-tabs\">";
  message += "<a class=\"nav-item nav-link" + String(((page==PAGE_ROOT) ? " active" : "")) + "\" href=\"/\">Status</a>";
  message += "<a class=\"nav-item nav-link" + String(((page==PAGE_HISTORY) ? " active" : "")) + "\" href=\"/history\">History</a>";
  message += "<a class=\"nav-item nav-link" + String(((page==PAGE_CONFIG) ? " active" : "")) + "\" href=\"/config\">Config</a>";
  message += "<a class=\"nav-item nav-link" + String(((page==PAGE_UPLOAD) ? " active" : "")) + "\" href=\"/upload\">Upload</a>";
  message += "</nav>";

  return message;
}

void HTTPSrv::HTML_Footer(String message) {
  message += "</body></html>";
  server->send(200, "text/html", message);
}

void HTTPSrv::handleRoot() {
  char TmpSt[255];
  String RowColor;
  String message;

  message = HTML_Header(PAGE_ROOT, true, false);

  message += "<table class=\"table\" id=\"states\">";
  message += "<thead><tr><th scope=\"col\">Name<th scope=\"col\">State<th scope=\"col\">Actions</thead>";
  message += "<tbody>";

  data->dspUptime(TmpSt);
  message += "<tr " + bootstrapColor(BS_BLUE) + " id=\"uptime\"><td>Uptime</td><td>" + String(TmpSt) + "</td><td></td>";

  message += "<tr " + float2RowColor(-data->Wifi_RSSI, 70, 85) + " id=\"wifi_signal\"><td>WiFi Signal</td><td>" + String(data->Wifi_RSSI) + "</td><td></td>";

  if (data->HomieConnected) {
    strcpy(TmpSt, "Connected");
  }
  else
  {
    strcpy(TmpSt, "Not connected");
  }
  message += "<tr " + bool2RowColor(data->HomieConnected) + " id=\"homie_connected\"><td>Homie status</td><td>" + String(TmpSt) + "</td><td></td>";

  if (NTP.syncStatus() == 0)
    sprintf(TmpSt, "Synced - %04i%02i%02i (%i)", data->DT.year, data->DT.month, data->DT.day, data->DT.dayofyear);
  else
    strcpy(TmpSt, "Not synced");

  message += "<tr " + bool2RowColor(NTP.syncStatus() == 0) + " id=\"ntp_synced\"><td>NTP status</td><td>" + String(TmpSt) + "</td><td></td>";

  if (data->TL.err)
    strcpy(TmpSt, "Error");
  else
    strcpy(TmpSt, "Sensor OK");

  message += "<tr " + bool2RowColor(!data->TL.err) + " id=\"tl_ok\"><td>Sensor status</td><td>" + String(TmpSt) + "</td><td></td>";

  sprintf(TmpSt, "%3i", data->TL.cm);
  message += "<tr " + int2RowColor(data->TL.pct, 15, 25) + " id=\"tl_cm\"><td>Niveau (cm)</td><td>" + String(TmpSt) + "</td><td></td>";
  sprintf(TmpSt, "%4i", data->TL.l);
  message += "<tr " + int2RowColor(data->TL.pct, 15, 25) + " id=\"tl_l\"><td>Niveau (l)</td><td>" + String(TmpSt) + "</td><td></td>";
  sprintf(TmpSt, "%3i", data->TL.pct);
  message += "<tr " + int2RowColor(data->TL.pct, 15, 25) + " id=\"tl_pct\"><td>Niveau (pct)</td><td>" + String(TmpSt) + "</td><td></td>";

  message += "</tbody></table><p>";
  HTML_Footer(message);
}

void HTTPSrv::handleConfig() {
  //char TmpSt[255];
  String message;

  message  = HTML_Header(PAGE_CONFIG, true, false);
  message += "<br />";
  File f = LittleFS.open("/config.json", "r");
  if (f) {
    message += f.readString();
    f.close();
  } else {
    message += "Read error";
  }
  HTML_Footer(message);
}

void HTTPSrv::handleSave() {
  String message;

  data->SaveConfig();
  message  = "Config saved";
  server->send(200, "text/html", message);
  }

void HTTPSrv::handle200() {
  server->send(200);
  }

void HTTPSrv::handleGetUpload() {
  String message;

  message  = HTML_Header(PAGE_UPLOAD, true, false);
  message += "<br /><form method=\"post\" enctype=\"multipart/form-data\"><input class=\"form-control\" type=\"file\" name=\"name\">";
  message += "<br /><input class=\"btn btn-danger\" type=\"submit\" value=\"Upload\"></form>";
  HTML_Footer(message);
  }

void HTTPSrv::handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server->upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    //Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      //Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      //server->sendHeader("Location","/success.html");      // Redirect the client to the success page
      //server->send(303);
      server->send(200, "text/plain", "200: file uploaded to flash filesystem");
    } else {
      server->send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void HTTPSrv::handleNotFound(){
  if (!handleFileRead(server->uri())) {
    server->send(404, "text/plain", "404: Not Found");
  }
/*
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";
  for (uint8_t i=0; i<server->args(); i++){
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->send(404, "text/plain", message);
*/
}

String HTTPSrv::getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool HTTPSrv::handleFileRead(String path) { // send the right file to the client (if it exists)
  //Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server->streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    //Serial.println(String("\tSent file: ") + path);
    return true;
  }
  //Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}


void HTTPSrv::handleHistory() {
  String message;
  char Txt[64];
  const char *Months[12] = {"Jan", "Feb", "Mar", "Avr", "Mai", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  DynamicJsonDocument doc(8192);
  DynamicJsonDocument datasetdoc(8192);

  message  = HTML_Header(PAGE_HISTORY, true, true);
  
  message += "<div class=\"chart-container\" style=\"position: relative; height:40vh; width:80vw\"><canvas id=\"chart\"></canvas></div>";
  message += "<script>var ctx = document.getElementById('chart').getContext('2d'); var chart = new Chart(ctx,";

  doc.clear();
  doc["type"] = "line";
  
  JsonObject jsondata = doc.createNestedObject("data");
  JsonArray jsonlabels = jsondata.createNestedArray("labels");
  JsonArray jsondatasets = jsondata.createNestedArray("datasets");
  
  datasetdoc.clear();
  datasetdoc["label"] = "Level";
  datasetdoc["backgroundColor"] = "#dfa49f";
	datasetdoc["borderColor"] = "#c45850";
  JsonArray jsonvalues = datasetdoc.createNestedArray("data");
  
  // Do calculation
  data->ReadHistory();

  for (int m = 0; m < 12; m++)
    for (int d = 0; d < 10; d++) {
      sprintf(Txt, "%3s-%02i", Months[(m + data->DT.month)%12], 3*d+1);
      jsonlabels.add(Txt);
      jsonvalues.add(data->History[m][d]);
  }

  jsondatasets.add(datasetdoc);
  serializeJson(doc, message);
  doc.clear();

  message += ");</script>";

  HTML_Footer(message);
}
