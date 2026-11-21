"""
legal notice, privacy policy (politique de confidentialite), about us / programmer
biography, text sanitizer (strict lowercase, no accents), and gta 4 / circular creator
avatar loaders for douala run (maximus 3d).
(c) 2024-2026 pondi ashley rey maxim - all rights reserved / tous droits reserves.
"""

from __future__ import annotations

import unicodedata
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import numpy as np
import pygame
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageOps

ASSETS_DIR = Path(__file__).resolve().parent.parent / "assets"
CREATOR_PHOTO_PATH = ASSETS_DIR / "creator_profile.jpg"


def clean_ui_text(text: str) -> str:
    """Enforce strictly lowercase, accent-free ASCII English/French text everywhere."""
    if not text:
        return ""
    replacements = {
        "œ": "oe",
        "Œ": "oe",
        "æ": "ae",
        "Æ": "ae",
        "©": "(c)",
        "—": "-",
        "–": "-",
        "•": "/",
        "«": '"',
        "»": '"',
        "’": "'",
        "‘": "'",
        "“": '"',
        "”": '"',
        "∞": "inf",
        "↑": "up",
        "↓": "down",
    }
    for src, dst in replacements.items():
        text = text.replace(src, dst)
    decomposed = unicodedata.normalize("NFKD", text)
    stripped = "".join(ch for ch in decomposed if not unicodedata.combining(ch))
    ascii_only = stripped.encode("ascii", "ignore").decode("ascii")
    return ascii_only.lower()


CREATOR_NAME = "pondi ashley rey maxim"
GAME_BRAND = "douala run"
COPYRIGHT_LINE = "(c) 2024-2026 pondi ashley rey maxim - all rights reserved / tous droits reserves"


PRIVACY_POLICY_SECTIONS: List[Tuple[str, str]] = [
    (
        "1. presentation / introduction",
        "welcome to douala run (maximus 3d), a 3d game conceived and developed solo by "
        "pondi ashley rey maxim since year 1 (2024). / bienvenue dans douala run (maximus 3d), "
        "un jeu 3d concu et developpe en solo par pondi ashley rey maxim depuis annee 1 (2024). "
        "this privacy policy explains how your local game data is handled transparently.",
    ),
    (
        "2. zero data collection / aucune collecte de donnees",
        "douala run runs 100% offline on your computer. / douala run fonctionne 100% hors ligne "
        "sur votre ordinateur. no personal data, no device id, no location, no telemetry, and no "
        "tracking are ever sent to external servers or shared with third parties.",
    ),
    (
        "3. local save only / stockage local uniquement",
        "the game only saves your progress locally in 'maximus_save.json' inside the game folder: "
        "unlocked douala districts, stars, best scores, virtual fcfa coins, upgrades, outfits, "
        "audio mute setting, and legal acceptance. / sauvegarde locale uniquement dans maximus_save.json.",
    ),
    (
        "4. local media & assets / medias integres",
        "all 3d meshes, procedural textures, audio synthesis, and the creator profile picture "
        "(pondi ashley rey maxim) are stored locally. the game never accesses your camera, "
        "microphone, or personal files. / aucun acces camera, micro ou fichiers personnels.",
    ),
    (
        "5. user rights & reset / droits utilisateur et suppression",
        "you keep full control of your local data and can erase everything anytime by deleting "
        "'maximus_save.json'. / vous pouvez supprimer toutes vos donnees locales a tout moment "
        "en effacant le fichier maximus_save.json. author / auteur: pondi ashley rey maxim.",
    ),
]


LEGAL_DOCUMENT_SECTIONS: List[Tuple[str, str]] = [
    (
        "1. copyright & ownership / propriete intellectuelle",
        "douala run (maximus 3d), including its native c++17 engine, python opengl 3.3 renderer, "
        "art direction, npc system, and procedural audio, is the exclusive property of "
        "pondi ashley rey maxim. (c) 2024-2026 pondi ashley rey maxim - all rights reserved / "
        "tous droits reserves.",
    ),
    (
        "2. software license / licence utilisation (eula)",
        "by accepting this legal document, the author grants you a personal, non-exclusive, "
        "non-transferable license to play douala run on your machine. / en acceptant ce document "
        "juridique, toute redistribution commerciale, revente ou appropriation du code sans accord "
        "ecrit de pondi ashley rey maxim est strictement interdite.",
    ),
    (
        "3. douala artistic tribute / hommage a la ville de douala",
        "the districts and landmarks featured (akwa boulevard de la liberte, marche mboppi, "
        "rond-point deido & monument de la nouvelle liberte, pont du wouri, bonanjo, yellow taxis, "
        "benskins, and pedestrians) are an artistic cultural tribute to douala, cameroon. / "
        "hommage artistique et culturel a douala, cameroun, sans affiliation officielle tierce.",
    ),
    (
        "4. disclaimer / responsabilite",
        "this software is provided 'as is' as an independent solo programming project. / "
        "ce logiciel est fourni en l'etat dans le cadre d'un projet independant de programmation. "
        "the author is not liable for indirect damages. take regular breaks while playing.",
    ),
    (
        "5. mandatory acceptance / acceptation prealable obligatoire",
        "access to douala run requires reading and explicitly accepting both the privacy policy "
        "and this legal document. / l'acces au jeu necessite l'acceptation explicite de la "
        "politique de confidentialite et du present document juridique.",
    ),
]


ABOUT_CONCEPTION_PARAGRAPHS: List[str] = [
    "conceived in year 1 (2024) under pure solo passion for programming, douala run (maximus 3d) "
    "was born from a bold dream: building a complete 3d arcade runner from scratch using only "
    "c++ and python. / concu depuis annee 1 (2024) par pure passion de la programmation en solo.",
    "from custom c++17 physics, 2-bone realistic kinematics, lively douala pnj pedestrians, and "
    "39 procedural 3d meshes to opengl 3.3 soft shadows, tropical lighting, and makossa audio, "
    "every line celebrates akwa, marche mboppi, rond-point deido, pont du wouri, and bonanjo.",
]


PROGRAMMER_BIO_LINES: List[str] = [
    "pondi ashley rey maxim - solo creator, engine programmer & designer / createur solo.",
    "started the conception of douala run in year 1 (2024), driven by passion for programming "
    "in solo, 3d graphics, and bringing the streets of douala cameroon to life. / "
    "conception lancee en annee 1 (2024) par passion de la programmation en solo.",
    "tech / stack: native c++17 (pybind11 physics, kinematics, pnj & 3d meshes) + python (opengl 3.3 / moderngl / pygame).",
]


_AVATAR_CACHE: Dict[Tuple[int, int], pygame.Surface] = {}
_GTA4_CACHE: Dict[Tuple[int, int], pygame.Surface] = {}


def load_creator_avatar(diameter: int, ring_width: int = 4) -> Optional[pygame.Surface]:
    """Load, smart-crop, circular-mask, and gold-emerald ring-frame the creator's portrait."""
    key = (diameter, ring_width)
    if key in _AVATAR_CACHE:
        return _AVATAR_CACHE[key]

    if not CREATOR_PHOTO_PATH.exists():
        return None

    try:
        src = Image.open(CREATOR_PHOTO_PATH).convert("RGBA")
        sw, sh = src.size
        side = int(min(sw, sh) * 0.76)
        cx = int(sw * 0.475)
        cy = int(sh * 0.50)
        left = max(0, min(sw - side, cx - side // 2))
        top = max(0, min(sh - side, cy - side // 2))
        cropped = src.crop((left, top, left + side, top + side))

        ss = 2
        total_d = diameter * ss
        rw = max(2, ring_width * ss)
        inner_d = max(8, total_d - rw * 2)

        portrait = cropped.resize((inner_d, inner_d), Image.Resampling.LANCZOS)
        mask = Image.new("L", (inner_d, inner_d), 0)
        ImageDraw.Draw(mask).ellipse([0, 0, inner_d - 1, inner_d - 1], fill=255)

        canvas = Image.new("RGBA", (total_d, total_d), (0, 0, 0, 0))
        draw = ImageDraw.Draw(canvas)
        draw.ellipse([0, 0, total_d - 1, total_d - 1], fill=(252, 206, 36, 255))
        if rw >= 6:
            pad = max(2, rw // 2)
            draw.ellipse([pad, pad, total_d - 1 - pad, total_d - 1 - pad], fill=(18, 172, 86, 255))

        canvas.paste(portrait, (rw, rw), mask)
        final_img = canvas.resize((diameter, diameter), Image.Resampling.LANCZOS)

        surf = pygame.image.frombytes(final_img.tobytes(), (diameter, diameter), "RGBA")
        _AVATAR_CACHE[key] = surf
        return surf
    except Exception:
        return None


def load_gta4_portrait(width: int = 320, height: int = 390) -> Optional[pygame.Surface]:
    """
    Create an iconic GTA IV-style illustrated loading-screen artwork panel of
    pondi ashley rey maxim (warm amber-noir grading, bold contrast, subtle inked contour,
    crisp dark frame with golden inner pinstripe).
    """
    key = (width, height)
    if key in _GTA4_CACHE:
        return _GTA4_CACHE[key]

    if not CREATOR_PHOTO_PATH.exists():
        return None

    try:
        src = Image.open(CREATOR_PHOTO_PATH).convert("RGB")
        sw, sh = src.size
        # Portrait crop matching requested aspect ratio centered on face & shoulders
        target_ratio = width / float(height)
        crop_h = int(sh * 0.92)
        crop_w = min(sw, int(crop_h * target_ratio))
        cx = int(sw * 0.475)
        cy = int(sh * 0.52)
        left = max(0, min(sw - crop_w, cx - crop_w // 2))
        top = max(0, min(sh - crop_h, cy - crop_h // 2))
        art = src.crop((left, top, left + crop_w, top + crop_h)).resize(
            (width, height), Image.Resampling.LANCZOS
        )

        # GTA IV illustrated painterly treatment: smooth skin planes + warm amber contrast + dark ink edges
        smooth = art.filter(ImageFilter.MedianFilter(size=3))
        poster = ImageOps.posterize(smooth, bits=5)
        blended = Image.blend(smooth, poster, alpha=0.38)
        blended = ImageEnhance.Contrast(blended).enhance(1.32)
        blended = ImageEnhance.Color(blended).enhance(1.18)

        # Subtle warm GTA IV golden-noir color tint & dark edge vignette
        arr = np.asarray(blended, dtype=np.float32)
        # Warm amber highlight lift + gritty cool shadow pull
        arr[:, :, 0] = np.clip(arr[:, :, 0] * 1.08 + 6.0, 0, 255)
        arr[:, :, 1] = np.clip(arr[:, :, 1] * 1.02 + 2.0, 0, 255)
        arr[:, :, 2] = np.clip(arr[:, :, 2] * 0.88, 0, 255)

        # Subtle dark comic ink contours from Sobel/edge pass
        edges = smooth.convert("L").filter(ImageFilter.FIND_EDGES)
        e_arr = np.asarray(edges, dtype=np.float32)
        ink = np.clip((e_arr - 34.0) / 140.0, 0.0, 0.55)
        arr *= (1.0 - ink[:, :, None])

        # Radial GTA IV vignette
        ys = np.linspace(-1.0, 1.0, height)[:, None]
        xs = np.linspace(-1.0, 1.0, width)[None, :]
        vig = np.clip(1.0 - 0.42 * (xs * xs + ys * ys), 0.42, 1.0)
        arr *= vig[:, :, None]

        rgba = np.zeros((height, width, 4), dtype=np.uint8)
        rgba[:, :, :3] = np.clip(arr, 0, 255).astype(np.uint8)
        rgba[:, :, 3] = 255
        card = Image.fromarray(rgba, mode="RGBA")

        # Iconic thick GTA IV black outer panel border + warm gold inner pinstripe
        draw = ImageDraw.Draw(card)
        draw.rectangle([0, 0, width - 1, height - 1], outline=(10, 10, 14, 255), width=8)
        draw.rectangle([8, 8, width - 9, height - 9], outline=(246, 194, 42, 240), width=2)

        surf = pygame.image.frombytes(card.tobytes(), (width, height), "RGBA")
        _GTA4_CACHE[key] = surf
        return surf
    except Exception:
        return None
