ffmpeg -y -r 60 -i data/screenshots/s%06d.png -c:v libx264 -preset veryslow -r 60 -pix_fmt yuv420p -crf 27 data/screenshots/out.mp4
