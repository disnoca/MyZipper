$buildDirectory = "build"

Remove-Item -Path $buildDirectory -Recurse -Force

Write-Host "`nBuilding zipper..."
& .\buildzipper.ps1 -SkipExistingObjectCompilation

Write-Host "`nBuilding unzipper..."
& .\buildunzipper.ps1 -SkipExistingObjectCompilation

Write-Host "`nBuilding zip info..."
& .\buildzipinfo.ps1 -SkipExistingObjectCompilation

Write-Host "`nAll builds completed."
