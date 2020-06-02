# /!\ THIS SCRIPT DOESN'T WORK TODAY BECAUSE OF SSF /!\
$isoPath = Join-Path $pwd.Path game.iso
$ssfDirectory = Join-Path $pwd.Path "..\..\Emulators\SSF\"
Write-Host "Mounting image..."
Mount-DiskImage -StorageType ISO -ImagePath $isoPath
Write-Host "Running SSF..."
cd "$ssfDirectory"
Start-Process -FilePath "SSF.exe" -Wait
Write-Host "Unmounting image..."
Get-DiskImage $isoPath | Dismount-DiskImage
