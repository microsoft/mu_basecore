# Repository Remote Check - Pre-Build Git Repository Validation Plugin

This plugin validates git repositories against remote commit criteria before the build starts. It allows you to define
rules that check whether the current HEAD commit in specified repositories is present or absent in specified remote
URLs.

This is useful for enforcing that your local repositories are using commits from the correct upstream sources, and
not from incompatible forks or remotes. For example, some workspaces might use a "public" and an "internal" upstream
and need to verify that both work as expected when used for a given platform build.

**Important**:

- This check verifies commit hashes. Therefore, the hashes will need to be consistent between the two repositories
  being compared.
- This plugin is enabled by adding the `remote_repo_check` scope to the scopes reported by the platform in
  `GetActiveScopes()`. It is NOT enabled by default (i.e. it does not use the `global` scope).

## Integration Instructions

This plugin serves a special purpose so it is not enabled by default. As mentioned above, it is enabled by activating
the `remote_repo_check` scope. You can enable only running this plugin in a server-side CI build, by adding the
following logic to the `GetActiveScopes()` implementation:

```python
        # Add remote_repo_check scope if running in server CI.
        #   - TF_BUILD is set by Azure DevOps pipelines
        #   - GITHUB_ACTIONS is set in GitHub workflow execution
        if os.environ.get('TF_BUILD') or os.environ.get('GITHUB_ACTIONS'):
            ps.extend(['remote_repo_check'])
```

This is recommended to allow developers to customize the repo (e.g. submodule) as needed locally while developing and
testing.

## Configuration

The plugin can be configured via a YAML or JSON configuration file. The configuration file path can be provided in two
ways:

1. **Command line**: `REPO_REMOTE_CHECK_CONFIG_PATH=<PATH>`
2. **PlatformBuild.py**: In the `SetPlatformEnv()` method:

   ```python
   self.env.SetValue("REPO_REMOTE_CHECK_CONFIG_PATH", <PATH>, "Platform Hardcoded")
   ```

If no configuration path is provided, the plugin looks for `RepoRemoteCheckConfig.yml` in the workspace root. If this
file is not found, the plugin exits successfully and does not perform any validation.

## Configuration File Format

The configuration file defines a list of repositories and their validation conditions for the current `HEAD` commit
in that repository path.

### YAML Example

```yaml
repositories:
  - path: "Common/MU_BASECORE"
    conditions:
      - type: "present_in_remote"
        remote: "https://github.com/microsoft/mu_basecore.git"
      - type: "not_present_in_remote"
        remote: "https://github.com/tianocore/edk2.git"

  - path: "Silicon/INTEL/IntelSiliconPkg"
    conditions:
      - type: "present_in_remote"
        remote: "https://github.com/tianocore/edk2-platforms.git"
```

### JSON Example

```json
{
  "repositories": [
    {
      "path": "Common/MU_BASECORE",
      "conditions": [
        {
          "type": "present_in_remote",
          "remote": "https://github.com/microsoft/mu_basecore.git"
        },
        {
          "type": "not_present_in_remote",
          "remote": "https://github.com/tianocore/edk2.git"
        }
      ]
    },
    {
      "path": "Silicon/INTEL/IntelSiliconPkg",
      "conditions": [
        {
          "type": "present_in_remote",
          "remote": "https://github.com/tianocore/edk2-platforms.git"
        }
      ]
    }
  ]
}
```

## Configuration Keys

### Top Level

- **repositories** (required): A list of repository configurations

### Repository Level

- **path** (required): Relative path from workspace root to the repository directory
- **conditions** (optional): A list of conditions to check against this repository

### Condition Level

- **type** (required): The type of check to perform. Valid values are:
  - `present_in_remote`: Verifies that the current `HEAD` commit exists in the specified remote
  - `not_present_in_remote`: Verifies that the current `HEAD` commit does NOT exist in the specified remote
- **remote** (required): The remote git URL to check against

## How It Works

1. The plugin reads the configuration file
2. For each repository in the configuration:
   - Validates that the repository path exists
   - Validates that the path is a git repository
   - Gets the current `HEAD` commit SHA in the local repository
   - Evaluates each condition for the repository path
     - Validates that the result matches the condition type
3. Returns success (`0`) if all conditions pass, failure (`1`) if any condition
   fails

**Note:** The plugin checks if a commit is *reachable* from any branch in the
remote repository. This means the commit must exist in the remote's history and
be part of at least one branch. Commits that exist in the remote but are not
part of any branch (orphaned commits, dangling commits) will not be detected.

## Common Errors

### Repository path does not exist: \<path\>

The specified repository path does not exist in the workspace. Check that:

- The path is correct and relative to the workspace root
- The repository has been cloned/initialized
- Stuart dependencies have been fetched

### Path is not a valid git repository: \<path\>

The specified path exists but is not a git repository. Ensure:

- The path points to a directory containing a `.git` folder
- The repository was properly initialized

### CONDITION FAILED: Repository '\<path\>' commit \<sha\> is NOT present in remote '\<url\>' (expected: present)

The current `HEAD` commit in the repository does not exist in the specified remote,
or is not reachable from any branch in that remote. This could mean:

- You're using a local commit that hasn't been pushed
- You're using a commit from a different fork
- The repository is on the wrong branch/commit
- The commit exists but is not part of any branch (orphaned/dangling commit)

**Solution**: Check the commit history and ensure you're using the correct commit
from the expected remote.

### CONDITION FAILED: Repository '\<path\>' commit \<sha\> IS present in remote '\<url\>' (expected: not present)

The current `HEAD` commit exists in a remote where it shouldn't, and is reachable
from at least one branch in that remote. This could mean:

- You're accidentally using commits from an incompatible upstream
- The repository is tracking the wrong remote

**Solution**: Verify the repository's current state and rebase/reset to the
correct commit.

### Invalid git remote URL: \<url\>

The specified remote URL is not a valid git repository URL. Check:

- The URL is correct and accessible
- You have network connectivity
- Authentication is properly configured if the remote requires it
