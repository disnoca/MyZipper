$srcDirectory = "src"
$buildDirectory = "build"
$objsDirectory = Join-Path -Path $buildDirectory -ChildPath "objs"
$targetExecutable = "zipper"

# Create the build directory if it doesn't exist
if (-not (Test-Path -Path $buildDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $buildDirectory | Out-Null
}

# Create the objs directory if it doesn't exist
if (-not (Test-Path -Path $objsDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $objsDirectory | Out-Null
}

# Get all the C files in the source directory
$cFiles = Get-ChildItem -Recurse -Path $srcDirectory -Filter "*.c" -File

# Compile each C file into an object file
foreach ($cFile in $cFiles) {
    $objectFile = Join-Path -Path $objsDirectory -ChildPath ($cFile.BaseName + ".o")
    $compileCommand = "gcc -c {0} -o {1}" -f $cFile.FullName, $objectFile
    Write-Host "Compiling $($cFile.Name)"
    Invoke-Expression -Command $compileCommand
}

# Create the target executable
$objectFiles = Get-ChildItem -Path $objsDirectory  -Filter "*.o" -File | Select-Object -ExpandProperty FullName
$linkCommand = "gcc {0} -o {1}" -f ($objectFiles -join " "), (Join-Path -Path $buildDirectory -ChildPath $targetExecutable)
Write-Host "Linking object files"
Invoke-Expression -Command $linkCommand

Write-Host "Compilation and linking completed."
