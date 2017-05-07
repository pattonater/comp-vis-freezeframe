# comp-vis-freezeframe


FFMPEG:

Turning a video into a sequence of frames:
ffmpeg -i input.m4v -vf fps=24 output%d.jpg

Turning a sequence of frames into a video:
ffmpeg -framerate 24 -i img%d.jpg output.m4v