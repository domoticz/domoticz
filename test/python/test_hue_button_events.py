"""Compile the actual Hue button handler against a small sensor/sender fixture.

Linux prerequisites: g++, JsonCpp development headers/library.
Run directly with Python; this is not physical Hue or full-server validation.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


class HueButtonEvents(unittest.TestCase):
    def test_button_metadata(self):
        if not shutil.which("g++"):
            self.skipTest("g++ required")
        root = pathlib.Path(__file__).resolve().parents[2]
        source = (root / "hardware/PhilipsHue/PhilipsHue.cpp").read_text()
        start = source.index("void CPhilipsHue::HandleSSEButton(")
        opening = source.index("{", start)
        depth = 1
        end = opening + 1
        while depth:
            depth += (source[end] == "{") - (source[end] == "}")
            end += 1
        fixture = root / "test/cpp"
        code = ((fixture / "hue_button_prefix.cpp").read_text()
                + source[start:end]
                + (fixture / "hue_button_cases.cpp").read_text())
        with tempfile.TemporaryDirectory() as folder:
            directory = pathlib.Path(folder)
            (directory / "test.cpp").write_text(code)
            subprocess.run(["g++", "-std=c++17", "-I/usr/include/jsoncpp",
                            str(directory / "test.cpp"), "-ljsoncpp", "-o",
                            str(directory / "test")], check=True, timeout=30)
            subprocess.run([str(directory / "test")], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
