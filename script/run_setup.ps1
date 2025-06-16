# 请求管理员权限
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

# 设置控制台标题
$Host.UI.RawUI.WindowTitle = "SGuard-Perfer"

# 获取脚本目录
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# 设置当前目录
Set-Location -Path $scriptDir

Write-Host "Setting Beginning ...`n"

# 配置 SGuardSvc64.exe
Write-Host "[Setting SGuardSvc64.exe]" -ForegroundColor Yellow
Start-Process -FilePath "$scriptDir\ProcessModifier.exe" -ArgumentList "SGuardSvc64.exe", "idle", "last4", "--eco" -Wait -NoNewWindow

Write-Host ""

# 配置 SGuard64.exe
Write-Host "[Setting SGuard64.exe]" -ForegroundColor Yellow
Start-Process -FilePath "$scriptDir\ProcessModifier.exe" -ArgumentList "SGuard64.exe", "idle", "last4", "--eco" -Wait -NoNewWindow

Write-Host "`nSetting Complete! Press any key to exit..." -ForegroundColor Green
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
