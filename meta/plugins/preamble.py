import subprocess
import requests

def build_ucd_cpp():
    subprocess.run([
        "python3",
        "src/libs/karm-icu/defs/build_ucd_cpp.py",
        "src/libs/karm-icu/ucd.cpp"
    ], check=True)


def download_to(url, path):
    with requests.get(url, stream=True) as r:
        r.raise_for_status() 
        with open(path, "wb") as f:
            for line_bytes in r.iter_lines():
                f.write(line_bytes)
  


def build_preamble():
    build_ucd_cpp()
    
    # Unicode Bidi Character Test
    download_to(
        "https://www.unicode.org/Public/16.0.0/ucd/BidiCharacterTest.txt", 
        "src/libs/karm-icu/tests/res/BidiCharacterTest.txt"
    )



if __name__ == "__main__":
    build_preamble()
