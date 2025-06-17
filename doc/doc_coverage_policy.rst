Documentation Coverage Policy
=============================

New tests should exercise public APIs covered in the reference
manual. The ``scripts/test_doc_coverage.py`` helper scans the specified
C sources for uses of exported functions declared in ``tiffio.h`` and
reports any that lack a corresponding ``.rst`` page under
``doc/functions``.

Policy
------

* CI may invoke ``test_doc_coverage.py`` on newly added tests.
  Missing documentation for used APIs will cause the job to fail.
* Add a ``doc/functions/<API>.rst`` page before exercising a new API
  in the test suite.
