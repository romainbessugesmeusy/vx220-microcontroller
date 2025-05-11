# Set ESP-IDF path
$env:IDF_PATH = "C:\Users\PC\Desktop\esp-idf"

# Run the ESP-IDF export script
& "$env:IDF_PATH\export.ps1"

# Print confirmation
Write-Host "ESP-IDF environment has been set up!"
Write-Host "IDF_PATH is set to: $env:IDF_PATH"
Write-Host "You can now run 'idf.py build'" 