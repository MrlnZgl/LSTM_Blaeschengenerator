function updateChart(remainingVolume) {
  const now = new Date().toLocaleTimeString();
  timeData.push(now);
  volumeData.push(remainingVolume);

  if (timeData.length > 60) { // Keep only the last 60 values
    timeData.shift();
    volumeData.shift();
  }
  
  // Store the latest data point in Local Storage
  localStorage.setItem('lastRemainingVolume', remainingVolume);

  volumeChart.update();
}

window.onload = function() {
  // Check if there's stored volume data
  const savedVolume = localStorage.getItem('lastRemainingVolume');
  if (savedVolume !== null) {
    // Use the stored volume to set initial state of your chart
    const initialVolume = parseFloat(savedVolume);
    updateChart(initialVolume);
  } else {
    // Initialize with a default value if nothing is stored
    const defaultVolume = 1000; // or any proper initial value you prefer
    updateChart(defaultVolume);
  }
  
  // Start fetching data periodically
  setInterval(fetchVolumeFlow, 5000); // Every 5 seconds as per your code
};
