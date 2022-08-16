#!/usr/bin/env powershell
$lastSvnRevision = 6962
$lastSvnHash = '16cd907fe7482cb54a7374cd28b8501f138116be'
$defineNumberMatch = [regex] '^#define\s+(\w+)\s+(\d+)$'
$defineStringMatch = [regex] "^#define\s+(\w+)\s+[`"']?(.+?)[`"']?$"
$semVerMatch = [regex] 'v?(\d+)\.(\d+).(\d+)(?:-(\w+))?'


$repositoryRootPath = Join-Path $PSScriptRoot .. | Resolve-Path
if (!(git -C $repositoryRootPath rev-parse --is-inside-work-tree 2>$null)) {
  throw "$repositoryRootPath is not a git repository"
}

$gitVersionHeaderPath = Join-Path $repositoryRootPath 'src' | Join-Path -ChildPath 'include' | Join-Path -ChildPath 'aegisub' `
  | Join-Path -ChildPath 'git_version.h'
$gitVersionXmlPath = Join-Path $repositoryRootPath 'packages' | Join-Path -ChildPath 'win_installer' `
  | Join-Path -ChildPath 'git_version.xml'

$version = @{}
if (Test-Path $gitVersionHeaderPath) {
  Get-Content $gitVersionHeaderPath | %{$_.Trim()} | ?{$_} | %{
    switch -regex ($_) {
      $defineNumberMatch {
        $version[$Matches[1]] = [int]$Matches[2];
      }
      $defineStringMatch {
        $version[$Matches[1]] = $Matches[2];
      }
    }
  }
}

$gitRevision = $lastSvnRevision + ((git -C $repositoryRootPath log --pretty=oneline "$($lastSvnHash)..HEAD" 2>$null | Measure-Object).Count)
$gitBranch = git -C $repositoryRootPath symbolic-ref --short HEAD 2>$null
$gitHash = git -C $repositoryRootPath rev-parse --short HEAD 2>$null
$gitVersionString = $gitRevision, $gitBranch, $gitHash -join '-'
$exactGitTag = git -C $repositoryRootPath describe --exact-match --tags 2>$null

if ($exactGitTag -match $semVerMatch) {
  $version['TAGGED_RELEASE'] = $true
  $version['RESOURCE_BASE_VERSION'] = $Matches[1..3]
  $version['INSTALLER_VERSION'] = $gitVersionString = ($Matches[1..3] -join '.') + @("-$($Matches[4])",'')[!$Matches[4]]
} else {
  foreach ($rev in (git -C $repositoryRootPath rev-list --tags 2>$null)) {
    $tag = git -C $repositoryRootPath describe --exact-match --tags $rev 2>$null
    if ($tag -match $semVerMatch) {#
      $version['TAGGED_RELEASE'] = $false
      $version['RESOURCE_BASE_VERSION'] = $Matches[1..3] + $gitRevision
      $version['INSTALLER_VERSION'] = ($Matches[1..3] -join '.') + "-" + $gitBranch
      break;
    }
  }
}

$version['BUILD_GIT_VERSION_NUMBER'] = $gitRevision
$version['BUILD_GIT_VERSION_STRING'] = $gitVersionString

$version.GetEnumerator() | %{
  $type = $_.Value.GetType()
  $value = $_.Value
  $fmtValue = switch ($type) {
    ([string]) {"`"$value`""}
    ([int]) {$value.ToString()}
    ([bool]) {([int]$value).ToString()}
    ([object[]]) {$value -join ', '}
    default {
      Write-Host "no format known for type '$type' - trying default string conversion" -ForegroundColor Red
      {"`"$($value.ToString())`""}
    }
  }
  "#define $($_.Key) $($fmtValue)"
} | Out-File -FilePath $gitVersionHeaderPath -Encoding utf8

$gitVersionXml = [xml]@'
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="12.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <GitVersionNumber></GitVersionNumber>
    <GitVersionString></GitVersionString>
  </PropertyGroup>
</Project>
'@

$gitVersionXml.Project.PropertyGroup.GitVersionNumber = $gitRevision.ToString()
$gitVersionXml.Project.PropertyGroup.GitVersionString = $gitVersionString
$gitVersionXml.Save($gitVersionXmlPath)
