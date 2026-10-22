import numpy as np
import pytest
from example.rotation import rot2d


def test_quarter_turn():
    np.testing.assert_allclose(rot2d(np.pi / 2) @ [1, 0], [0, 1], atol=1e-12)


def test_orthonormal():
    R = rot2d(0.3)
    np.testing.assert_allclose(R.T @ R, np.eye(2), atol=1e-12)


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-q"]))
