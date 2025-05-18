package integration_test

import (
	"context"
	"os"
	"testing"
	"time"

	"github.com/docker/docker/client"
	"github.com/stretchr/testify/require"
	"github.com/testcontainers/testcontainers-go"
	"github.com/testcontainers/testcontainers-go/wait"
)

const (
	imageName = "python-service:latest"
)

func loadLatestTarball(t *testing.T) {
	tarPath := os.Getenv("TEST_SRCDIR") + "/" + os.Getenv("TARBALL_RUNFILE")
	cli, err := client.NewClientWithOpts(client.FromEnv)
	require.NoError(t, err)

	file, err := os.Open(tarPath)
	require.NoError(t, err)
	defer file.Close()

	_, err = cli.ImageLoad(context.Background(), file, true)
	require.NoError(t, err)
}

func TestPythonServiceContainerRuns(t *testing.T) {
	ctx := context.Background()

	// Load the latest tarball image into Docker daemon
	loadLatestTarball(t)

	req := testcontainers.ContainerRequest{
		Image:        imageName,
		ExposedPorts: []string{"8080/tcp"},
		WaitingFor:   wait.ForLog("Hello World!").WithStartupTimeout(2 * time.Minute),
		// Bind port 8080 in container to 9000 on host
		PortBindings: map[string][]string{"8080/tcp": {"9000"}},
	}

	container, err := testcontainers.GenericContainer(ctx, testcontainers.GenericContainerRequest{
		ContainerRequest: req,
		Started:          true,
	})
	require.NoError(t, err)
	defer container.Terminate(ctx)

	// Get the mapped port on the host machine
	hostPort, err := container.MappedPort(ctx, "8080")
	require.NoError(t, err)

	t.Logf("Container is ready and mapped port is %s", hostPort.Port())
}
