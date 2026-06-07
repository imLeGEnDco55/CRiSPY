import sys
import os

# Auto-install dependencies if import fails
try:
    import grpc
    import soundfile as sf
    import numpy as np
except ImportError:
    print("Missing python packages, installing from requirements.txt...")
    import subprocess
    requirements_path = os.path.join(os.path.dirname(__file__), "requirements.txt")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", requirements_path])
        import grpc
        import soundfile as sf
        import numpy as np
        print("Installation complete.")
    except Exception as e:
        print(f"Error installing dependencies: {e}", file=sys.stderr)
        sys.exit(1)

import argparse
import tempfile
from typing import Iterator

# Add interfaces to path
sys.path.append(os.path.join(os.path.dirname(__file__), "interfaces", "studio_voice"))
import studiovoice_pb2
import studiovoice_pb2_grpc

def read_audio_robust(input_path: str):
    """
    Attempts to read audio file, with fallback for Vorbis-in-WAV or files with unknown frame counts.
    """
    try:
        return sf.read(input_path)
    except Exception as read_err:
        try:
            with open(input_path, 'rb') as f:
                content = f.read()
            
            ogg_start = content.find(b'OggS')
            if ogg_start != -1:
                temp_fd, temp_ogg_path = tempfile.mkstemp(suffix=".ogg")
                try:
                    with os.fdopen(temp_fd, "wb") as f_ogg:
                        f_ogg.write(content[ogg_start:])
                    
                    with sf.SoundFile(temp_ogg_path) as f_sf:
                        # If frame count is unreasonably huge, read in blocks
                        if f_sf.frames > 10**12 or f_sf.frames < 0:
                            blocks = []
                            while True:
                                block = f_sf.read(1024)
                                if len(block) == 0:
                                    break
                                blocks.append(block)
                            data = np.concatenate(blocks, axis=0)
                        else:
                            data = f_sf.read()
                        samplerate = f_sf.samplerate
                        return data, samplerate
                finally:
                    if os.path.exists(temp_ogg_path):
                        try:
                            os.remove(temp_ogg_path)
                        except:
                            pass
        except Exception:
            pass
        raise read_err

def normalize_audio(input_path: str, temp_out_path: str):
    """
    Reads input audio, downmixes to mono, resamples to 48000Hz, and writes as 16-bit WAV PCM.
    """
    try:
        data, samplerate = read_audio_robust(input_path)
    except Exception as e:
        raise ValueError(f"Could not read input audio: {e}")

    # Downmix to mono if stereo
    if len(data.shape) > 1:
        data = np.mean(data, axis=1)

    # Resample to 48000 Hz if necessary
    target_rate = 48000
    if samplerate != target_rate:
        duration = len(data) / samplerate
        num_samples = int(duration * target_rate)
        xp = np.arange(len(data))
        x = np.linspace(0, len(data) - 1, num_samples)
        data = np.interp(x, xp, data)
        samplerate = target_rate

    # Write as 16-bit PCM WAV
    sf.write(temp_out_path, data, target_rate, subtype='PCM_16')

def generate_chunks(filepath: str) -> Iterator[studiovoice_pb2.EnhanceAudioRequest]:
    DATA_CHUNKS = 64 * 1024  # 64 KB
    with open(filepath, "rb") as fd:
        while True:
            buffer = fd.read(DATA_CHUNKS)
            if buffer == b"":
                break
            yield studiovoice_pb2.EnhanceAudioRequest(audio_stream_data=buffer)

def main():
    parser = argparse.ArgumentParser(description="Clean audio using Nvidia Studio Voice NIM")
    parser.add_argument("--input", required=True, help="Path to input audio file")
    parser.add_argument("--output", required=True, help="Path to save cleaned audio file")
    parser.add_argument("--api-key", required=True, help="Nvidia API Key")
    args = parser.parse_args()

    input_path = args.input
    output_path = args.output
    api_key = args.api_key

    # Temp files in the same directory as output
    output_dir = os.path.dirname(os.path.abspath(output_path))
    temp_input_path = os.path.join(output_dir, "temp_normalized_input.wav")

    print(f"Normalizing input: {input_path} -> {temp_input_path}...")
    try:
        normalize_audio(input_path, temp_input_path)
    except Exception as e:
        print(f"Normalization failed: {e}", file=sys.stderr)
        sys.exit(1)

    print("Connecting to NVIDIA Studio Voice Service...")
    function_id = "3f0aeba3-6d91-4465-b8cc-cc2aef355186"
    request_metadata = (
        ("authorization", f"Bearer {api_key}"),
        ("function-id", function_id),
    )

    channel_credentials = grpc.ssl_channel_credentials()
    target = "grpc.nvcf.nvidia.com:443"

    try:
        with grpc.secure_channel(target=target, credentials=channel_credentials) as channel:
            stub = studiovoice_pb2_grpc.StudioVoiceStub(channel)
            print("Sending audio data chunks to Nvidia API...")
            
            # Start inference
            responses = stub.EnhanceAudio(
                generate_chunks(temp_input_path),
                metadata=request_metadata
            )
            
            print(f"Writing enhanced audio to {output_path}...")
            with open(output_path, "wb") as fd:
                for response in responses:
                    if response.HasField("audio_stream_data"):
                        fd.write(response.audio_stream_data)
                        
        print("Success: Audio cleaning finished.")
    except Exception as e:
        print(f"API Inference failed: {e}", file=sys.stderr)
        if os.path.exists(output_path):
            try:
                os.remove(output_path)
            except:
                pass
        sys.exit(1)
    finally:
        # Clean up temporary input file
        if os.path.exists(temp_input_path):
            try:
                os.remove(temp_input_path)
            except:
                pass

if __name__ == "__main__":
    main()
