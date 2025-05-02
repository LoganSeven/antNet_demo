# TODO.md

## Project Initialization Tasks

This file lists all the technical tasks required to establish the foundational structure of the AntNet Demo project. Tasks are written to be performed in order, and each can be checked off when complete.

---

### [x] 1. Create Project Skeleton

* [x] Set up base directory structure:

  * `src/python/core/`
  * `src/python/gui/`
  * `src/python/ffi/`
  * `src/c/` and `include/`
  * `tests/`, `build/`, `documentation/`
* [x] Initialize Git and add `.gitignore`, `README.md`, `LICENSE`, etc.

---

### [x] 2. Implement Minimal GUI

* [x] Create `MainWindow` using QtPy (with PyQt5 backend)
* [x] Add a button to stop all workers
* [x] Add label for iteration count
* [x] Add a label per worker to show best path

---

### [x] 3. Implement Core Architecture

* [x] Create `CoreManager` class
* [x] Create `Worker` class running in a `QThread`
* [x] Create `QCCallbackToSignal` for safe UI updates from threads

---

### [x] 4. Provide Mock C Backend

* [x] Define `AntNetContext` and `AntNetPathInfo` in `backend.h`
* [x] Implement mocked versions of:

  * `antnet_init()`
  * `antnet_run_iteration()`
  * `antnet_get_best_path_struct()`
  * `antnet_shutdown()`

---

### [x] 5. Integrate CFFI Binding

* [x] Write `ffi_build.py` to compile C backend into Python module
* [x] Write `backend_api.py` as a wrapper class
* [x] Map `AntNetPathInfo` to a Python dictionary

---

### [x] 6. Set Up Testing

* [x] Create `tests/test_backend_api.py`
* [x] Ensure it initializes, runs, verifies and shuts down the backend
* [x] Add `pytest.ini` and run all tests

---

### ☐ 7. Automate Build Process

* [x] Create `build.sh` for Linux/macOS
* [x] Create `build.bat` for Windows
* [ ] Optionally, create `Makefile` for `build`, `cffi`, `test`, `clean`

---

### ☐ 8. Final Verification

* [x] Confirm end-to-end execution via `python3 main.py`
* [x] Confirm unit test success via `python3 -m pytest`

---