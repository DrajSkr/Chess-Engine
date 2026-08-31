import urllib.request
import os

themes = ['cburnett', 'merida', 'alpha', 'fantasy']
pieces = ['wK', 'wQ', 'wR', 'wB', 'wN', 'wP', 'bK', 'bQ', 'bR', 'bB', 'bN', 'bP']
base_url = 'https://raw.githubusercontent.com/lichess-org/lila/master/public/piece/'

for t in themes:
    os.makedirs(f'frontend/public/pieces/{t}', exist_ok=True)
    for p in pieces:
        url = f'{base_url}{t}/{p}.svg'
        path = f'frontend/public/pieces/{t}/{p}.svg'
        try:
            urllib.request.urlretrieve(url, path)
            print(f'Downloaded {path}')
        except Exception as e:
            print(f'Error downloading {path}: {e}')
