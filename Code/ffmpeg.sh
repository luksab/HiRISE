ffmpeg -y -i data/screenshots/s%06d.png -c:v libx264 -preset veryslow -pix_fmt yuv420p -vf fps=60 -crf 20 data/screenshots/out.mp4
