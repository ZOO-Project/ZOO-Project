# `crane config` server

This server integrates the [crane](https://github.com/google/go-containerregistry/blob/main/cmd/crane/README.md) CLI tool within a Flask application to retrieve and return the JSON output from `crane config`.

To build and push the Docker image for multiple platforms, use:

```bash
docker buildx build \
    -t zooproject/crane-minimal:latest \
    --push \
    --platform linux/amd64,linux/arm64 .
```

When the environment variable `ZOOFPM_DETECT_ENTRYPOINT` is set to `true`, the server assists the `DeployProcess` service in checking whether any container referenced in a CWL file (`hints/DockerRequirement/dockerPull`) defines an `ENTRYPOINT`. If an image specifies an `ENTRYPOINT`, the `app-package.cwl` is automatically updated to prepend it to the existing `baseCommand`, or to create one if it does not exist.

This ensures consistent behavior for OGC Application Packages in CWL, whether deployed via ZOO-Project-DRU or executed with `cwltool`.