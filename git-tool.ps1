param(
    [ValidateSet('help', 'status', 'upload', 'pull', 'push', 'merge', 'sync', 'branches')]
    [string]$Action = 'help',

    [string]$Message = '',
    [string]$Branch = '',
    [string]$Remote = 'origin'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Run-Git {
    param([string[]]$GitArgs)
    Write-Host "`n> git $($GitArgs -join ' ')" -ForegroundColor Cyan
    & git -C $RepoRoot @GitArgs
    if ($LASTEXITCODE -ne 0) {
        throw "git command failed: git $($GitArgs -join ' ')"
    }
}

function Get-CurrentBranch {
    $name = & git -C $RepoRoot branch --show-current
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($name)) {
        throw 'Cannot detect current branch.'
    }
    return $name.Trim()
}

function Test-HasChanges {
    $status = & git -C $RepoRoot status --porcelain
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read git status.' }
    return -not [string]::IsNullOrWhiteSpace(($status -join "`n"))
}

function Show-Help {
    Write-Host @"
Git helper for TryComboWepon

Usage:
  .\git-tool.cmd status
  .\git-tool.cmd upload "message"
  .\git-tool.cmd pull
  .\git-tool.cmd push
  .\git-tool.cmd merge branch-name
  .\git-tool.cmd sync "message"
  .\git-tool.cmd branches

Actions:
  status    Show local changes and current branch.
  upload    git add -A, commit with message, push current branch.
  pull      Pull current branch with --ff-only.
  push      Push current branch to origin.
  merge     Fetch, then merge the given branch into current branch.
  sync      Pull first, then upload current changes.
  branches  Show local and remote branches.

Examples:
  .\git-tool.cmd upload "Add combat start menu"
  .\git-tool.cmd merge main
  .\git-tool.cmd merge origin/main
"@
}

if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot '.git'))) {
    throw "This script must stay in a git repository root. Current path: $RepoRoot"
}

switch ($Action) {
    'help' {
        Show-Help
    }
    'status' {
        Run-Git @('status', '-sb')
        Run-Git @('remote', '-v')
    }
    'branches' {
        Run-Git @('branch', '-vv')
        Run-Git @('branch', '-r')
    }
    'pull' {
        $current = Get-CurrentBranch
        Run-Git @('pull', '--ff-only', $Remote, $current)
    }
    'push' {
        $current = Get-CurrentBranch
        Run-Git @('push', '-u', $Remote, $current)
    }
    'upload' {
        $current = Get-CurrentBranch
        if (-not (Test-HasChanges)) {
            Write-Host 'No local changes to upload.' -ForegroundColor Yellow
            Run-Git @('status', '-sb')
            exit 0
        }

        if ([string]::IsNullOrWhiteSpace($Message)) {
            $Message = 'Update project files ' + (Get-Date -Format 'yyyy-MM-dd HH:mm')
        }

        Run-Git @('add', '-A')
        Run-Git @('status', '-sb')
        Run-Git @('commit', '-m', $Message)
        Run-Git @('push', '-u', $Remote, $current)
    }
    'merge' {
        if ([string]::IsNullOrWhiteSpace($Branch)) {
            throw 'Merge needs a branch name. Example: .\git-tool.cmd merge main'
        }

        Run-Git @('fetch', $Remote)
        Run-Git @('merge', $Branch)
        Write-Host "Merge finished. If this created a merge commit or changed files, run upload to push it." -ForegroundColor Green
    }
    'sync' {
        $current = Get-CurrentBranch
        Run-Git @('pull', '--ff-only', $Remote, $current)

        if (-not (Test-HasChanges)) {
            Write-Host 'Already up to date. No local changes to upload.' -ForegroundColor Green
            exit 0
        }

        if ([string]::IsNullOrWhiteSpace($Message)) {
            $Message = 'Update project files ' + (Get-Date -Format 'yyyy-MM-dd HH:mm')
        }

        Run-Git @('add', '-A')
        Run-Git @('status', '-sb')
        Run-Git @('commit', '-m', $Message)
        Run-Git @('push', '-u', $Remote, $current)
    }
}
