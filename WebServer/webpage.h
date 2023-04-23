const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
  <head>
    <meta charset="utf-8">
    <title>Domótica</title>
  </head>
  <style type="text/css">
  </style>
  <body style="background-color: #333333; color: white;" onload="getData()">
    <center>
      <div>
        <h1>Domótica - Kleysson Douglas de Faria</h1>
        <h2 id="state"> </h2>
        <button class="button" onclick="operateValve(1)">Válvula 1</button>
        <span id="valve1"> </span>
        <br>
        <button class="button" onclick="operateValve(2)">Válvula 2</button>
        <span id="valve2"> </span>
        <br>
      </div>
      <br>
      <div>
        <p>
          Atualizado em
          <span id="timestamp"> </span>
        </p>
        <p>
          Temperatura
          <span id="temperature"> </span> 
          ºC
        </p>
        <p>
          Umidade
          <span id="humidity"> </span> 
          %
        </p>
        <p>
          Humidade do Solo
          <span id="moisture"> </span> 
        </p>
      </div>
    </center>
  </body>
  
  <script>
    let timestampElement = document.getElementById("timestamp");
    let temperatureElement = document.getElementById("temperature");
    let humidityElement = document.getElementById("humidity");
    let moistureElement = document.getElementById("moisture");
    let stateElement = document.getElementById("state");
    let valve1Element = document.getElementById("valve1");
    let valve2Element = document.getElementById("valve2");

    function operateValve(valve) 
    {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          setData(this.responseText);
        }
      };
      xhttp.open("GET", "operate_valve?valve="+valve, true);
      xhttp.send();
    }

    setInterval(function() 
    {
      getData();
    }, 10000);

    function getData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          setData(this.responseText);
        }
      };
      xhttp.open("GET", "get_data", true);
      xhttp.send();
    }

    function setData(data) {
      let resp = JSON.parse(data);
      setElementValue(timestampElement, resp.timestamp);
      setElementValue(temperatureElement, resp.temperature);
      setElementValue(humidityElement, resp.humidity);
      setElementValue(moistureElement, resp.moisture == "1" ? "Seco" : "Molhado");
      setElementValue(stateElement, resp.state);
      setElementValue(valve1Element, resp.valve1);
      setElementValue(valve2Element, resp.valve2);
    }

    function setElementValue(element, value) {
      element.innerHTML = value;
    }
  </script>
</html>
)=====";