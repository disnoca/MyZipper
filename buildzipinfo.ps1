param (
    [switch]$SkipExistingObjectCompilation
)

$srcDirectory = "src"
$srcNestedDirectories = @("zip_info")

$buildDirectory = "build"
$objsDirectory = Join-Path -Path $buildDirectory -ChildPath "objs"
$targetExecutable = "zip_info"

$cFiles = @()
$objectFiles = @()

# Create the build directory if it doesn't exist
if (-not (Test-Path -Path $buildDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $buildDirectory | Out-Null
}

# Create the objs directory if it doesn't exist
if (-not (Test-Path -Path $objsDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $objsDirectory | Out-Null
}

# Get all the C files
$cFiles += Get-ChildItem -Path $srcDirectory -Filter "*.c" -File
foreach ($srcNestedDirectory in $srcNestedDirectories) {
    $cFiles += Get-ChildItem -Recurse -Path (Join-Path -Path $srcDirectory -ChildPath $srcNestedDirectory) -Filter "*.c" -File
}

# Compile each C file into an object file
foreach ($cFile in $cFiles) {
    $objectFile = Join-Path -Path $objsDirectory -ChildPath ($cFile.BaseName + ".o")
    $objectFiles += $objectFile

    if ($SkipExistingObjectCompilation -and (Test-Path -Path $objectFile)) {
        Write-Host "Skipping compilation for $($cFile.Name)"
    } else {
        $compileCommand = "gcc -c {0} -o {1}" -f $cFile.FullName, $objectFile
        Write-Host "Compiling $($cFile.Name)"
        Invoke-Expression -Command $compileCommand
    }
}

# Create the target executable
$linkCommand = "gcc {0} -o {1}" -f ($objectFiles -join " "), (Join-Path -Path $buildDirectory -ChildPath $targetExecutable)
Write-Host "Linking object files"
Invoke-Expression -Command $linkCommand

Write-Host "Compilation and linking completed"
