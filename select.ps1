param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateRange(1, [int]::MaxValue)]
    [int] $Id
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$workspace = (Resolve-Path $PSScriptRoot).Path
$problemsDirectory = Join-Path $workspace 'problems'
$matches = @(
    Get-ChildItem -LiteralPath $problemsDirectory -Directory |
        Where-Object { $_.Name -match "^$Id\." }
)

if ($matches.Count -eq 0) {
    throw "没有目录匹配 problems/$Id.*"
}
if ($matches.Count -ne 1) {
    throw "题号 $Id 对应多个目录：$($matches.Name -join ', ')"
}

$selectionFile = Join-Path $workspace 'current_problem.txt'
$current = if (Test-Path -LiteralPath $selectionFile) {
    (Get-Content -Raw -LiteralPath $selectionFile).Trim()
} else {
    ''
}

if ($current -ne $Id.ToString()) {
    [System.IO.File]::WriteAllText(
        $selectionFile,
        "$Id`n",
        [System.Text.UTF8Encoding]::new($false)
    )
}

Write-Host "已选择力扣题目 $Id :: $($matches[0].Name)"
