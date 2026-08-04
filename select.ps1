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

function Find-ProblemDirectory {
    @(
        Get-ChildItem -LiteralPath $problemsDirectory -Directory |
            Where-Object { $_.Name -match "^$Id\." }
    )
}

$matches = @(Find-ProblemDirectory)

if ($matches.Count -eq 0) {
    $fetchScript = Join-Path $workspace 'tools/fetch_problem.py'
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($null -eq $python) {
        throw "没有目录匹配 problems/$Id.*，并且找不到 Python；请手动运行 tools/fetch_problem.py"
    }

    Write-Host "本地没有题目 $Id，正在从力扣拉取..."
    & $python.Source $fetchScript $Id
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    $matches = @(Find-ProblemDirectory)
    if ($matches.Count -eq 0) {
        throw "拉取完成后仍没有目录匹配 problems/$Id.*"
    }
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
