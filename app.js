document.querySelector('#connect').addEventListener('click', connect);

const updateFrequency = 1000; // 1秒に1回まで更新
let lastUpdateTime = 0;

async function animate(element, value) {
  element.classList.add("changed");
  setTimeout(() => { element.classList.remove("changed"); }, 1000);
  element.textContent = value;
}

async function connect() {
  let characteristic;

  try {
    const device = await navigator.bluetooth.requestDevice({
      acceptAllDevices: true,
      optionalServices: ['4fafc201-1fb5-459e-8fcc-c5c9c331914b']
    });

    device.addEventListener('gattserverdisconnected', onDisconnected);

    const server = await device.gatt.connect();
    const service = await server.getPrimaryService('4fafc201-1fb5-459e-8fcc-c5c9c331914b');
    characteristic = await service.getCharacteristic('beb5483e-36e1-4688-b7f5-ea07361b26a8');

    characteristic.addEventListener('characteristicvaluechanged', handleValueChanged);

    await characteristic.startNotifications();
  } catch (error) {
    console.log('Failed:', error);
  }

  function onDisconnected(event) {
    const device = event.target;
    setTimeout(connect, 5000);
  }

  function handleValueChanged(event) {
    const now = Date.now();
    if (now - lastUpdateTime < updateFrequency) {
      return;
    }
    lastUpdateTime = now;

    const data = event.target.value;
    const dataView = new DataView(data.buffer);
    const cpuUsage = dataView.getUint32(0, true);
    const freeHeap = dataView.getUint32(4, true);
    const uptime = dataView.getUint32(8, true);
    const model = dataView.getUint8(12);
    const flashChipSize = dataView.getUint32(13, true);
    const resetReason = dataView.getUint32(17, true);
    const hallSensorValue = dataView.getUint32(21, true);

    animate(document.querySelector('#cpu_usage'), `${cpuUsage}`);
    animate(document.querySelector('#free_heap'), ` ${freeHeap} bytes`);
    animate(document.querySelector('#uptime'), ` ${uptime} ms`);
    animate(document.querySelector('#esp_model'), ` ${model}`);
    animate(document.querySelector('#flash_chip_size'), `${flashChipSize} bytes`);
    animate(document.querySelector('#reset_reason'), `${resetReason}`);
    animate(document.querySelector('#hall_sensor_value'), `${hallSensorValue}`);
  }
}
