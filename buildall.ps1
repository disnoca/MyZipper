$buildDirectory = "build"

# Clear the build directory
Remove-Item -Path $buildDirectory -Recurse -Force

# Run buildzipper.ps1
Write-Host "`nBuilding zipper..."
& .\buildzipper.ps1 -SkipExistingObjectCompilation

# Run buildunzipper.ps1
Write-Host "`nBuilding unzipper..."
& .\buildunzipper.ps1 -SkipExistingObjectCompilation

Write-Host "`nAll builds completed."
