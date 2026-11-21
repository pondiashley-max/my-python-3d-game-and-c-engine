"""
Persistent JSON progression, FCFA coin wallet, level unlocks, star ratings,
power-up upgrades, and outfit unlocks for Maximus: Douala Runner 3D.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List

from .levels import OUTFITS, get_level

SAVE_FILE = Path(__file__).resolve().parent.parent / "maximus_save.json"


class SaveManager:
    def __init__(self, path: Path = SAVE_FILE) -> None:
        self.path = path
        self.data = self._default_data()
        self.load()

    @staticmethod
    def _default_data() -> dict:
        return {
            "unlocked_level": 1,
            "all_unlocked_override": False,
            "total_coins": 120,  # Starter FCFA pocket coins
            "stars": {},         # str(level_id) -> int (0..3)
            "best_scores": {},   # str(level_id) -> int
            "best_endless_dist": 0.0,
            "upgrades": {
                "magnet": 1,      # Level 1..5
                "sneakers": 1,
                "jetpack": 1,
                "multiplier": 1,
            },
            "hoverboard_stock": 3,
            "outfit_id": 0,
            "unlocked_outfits": [0],
            "sound_muted": False,
            "legal_accepted": False,
            "legal_accepted_at": "",
        }

    def load(self) -> None:
        if not self.path.exists():
            return
        try:
            raw = json.loads(self.path.read_text(encoding="utf-8"))
            base = self._default_data()
            base.update(raw)
            if isinstance(raw.get("upgrades"), dict):
                u = self._default_data()["upgrades"]
                u.update(raw["upgrades"])
                base["upgrades"] = u
            self.data = base
        except Exception:
            self.data = self._default_data()

    def save(self) -> None:
        try:
            self.path.write_text(json.dumps(self.data, indent=2), encoding="utf-8")
        except Exception:
            pass

    def is_legal_accepted(self) -> bool:
        return bool(self.data.get("legal_accepted", False))

    def accept_legal(self) -> None:
        from datetime import datetime

        self.data["legal_accepted"] = True
        self.data["legal_accepted_at"] = datetime.now().isoformat(timespec="seconds")
        self.save()

    def is_level_unlocked(self, level_id: int) -> bool:
        if self.data.get("all_unlocked_override", False):
            return True
        if level_id == 6:  # Endless Tour unlocks after Level 1 OR always available with override
            return self.data["unlocked_level"] >= 2 or True  # Endless always playable for fun!
        return level_id <= self.data["unlocked_level"]

    def toggle_unlock_all(self) -> bool:
        cur = not self.data.get("all_unlocked_override", False)
        self.data["all_unlocked_override"] = cur
        self.save()
        return cur

    def get_stars(self, level_id: int) -> int:
        return int(self.data["stars"].get(str(level_id), 0))

    def get_best_score(self, level_id: int) -> int:
        return int(self.data["best_scores"].get(str(level_id), 0))

    def record_run(self, level_id: int, score: int, coins: int, distance: float,
                   completed: bool, stumbles: int) -> int:
        """Record run stats, bank FCFA coins, unlock next district, and return earned stars."""
        self.data["total_coins"] = int(self.data["total_coins"]) + int(coins)
        key = str(level_id)
        if score > self.get_best_score(level_id):
            self.data["best_scores"][key] = int(score)

        lvl = get_level(level_id)
        if lvl.is_endless and distance > float(self.data.get("best_endless_dist", 0.0)):
            self.data["best_endless_dist"] = round(float(distance), 1)

        earned_stars = 0
        if completed and not lvl.is_endless:
            earned_stars = 1
            if coins >= lvl.star2_coins:
                earned_stars = 2
            if coins >= lvl.star3_coins and stumbles == 0:
                earned_stars = 3
            elif coins >= int(lvl.star3_coins * 1.15):
                earned_stars = 3

            if earned_stars > self.get_stars(level_id):
                self.data["stars"][key] = earned_stars
            if level_id < 5 and self.data["unlocked_level"] <= level_id:
                self.data["unlocked_level"] = level_id + 1

        self.save()
        return earned_stars

    def upgrade_cost(self, key: str) -> int:
        lvl = int(self.data["upgrades"].get(key, 1))
        if lvl >= 5:
            return 0
        return 80 * lvl

    def buy_upgrade(self, key: str) -> bool:
        cost = self.upgrade_cost(key)
        if cost <= 0 or self.data["total_coins"] < cost:
            return False
        self.data["total_coins"] -= cost
        self.data["upgrades"][key] = int(self.data["upgrades"].get(key, 1)) + 1
        self.save()
        return True

    def buy_hoverboard_pack(self) -> bool:
        cost = 60
        if self.data["total_coins"] < cost:
            return False
        self.data["total_coins"] -= cost
        self.data["hoverboard_stock"] = min(20, int(self.data.get("hoverboard_stock", 2)) + 2)
        self.save()
        return True

    def select_or_buy_outfit(self, outfit_id: int) -> bool:
        unlocked: List[int] = list(self.data.get("unlocked_outfits", [0]))
        if outfit_id in unlocked:
            self.data["outfit_id"] = outfit_id
            self.save()
            return True
        outfit = next((o for o in OUTFITS if o["id"] == outfit_id), None)
        if not outfit:
            return False
        cost = int(outfit["cost"])
        if self.data["total_coins"] < cost:
            return False
        self.data["total_coins"] -= cost
        unlocked.append(outfit_id)
        self.data["unlocked_outfits"] = unlocked
        self.data["outfit_id"] = outfit_id
        self.save()
        return True

    def to_engine_upgrades(self, eng_mod):
        u = eng_mod.UpgradeParams()
        ups: Dict[str, int] = self.data["upgrades"]
        u.magnet_duration = 8.0 + 2.5 * ups.get("magnet", 1)
        u.sneakers_duration = 8.0 + 2.5 * ups.get("sneakers", 1)
        u.jetpack_duration = 6.0 + 1.5 * ups.get("jetpack", 1)
        u.multiplier_duration = 10.0 + 3.0 * ups.get("multiplier", 1)
        u.starting_hoverboards = int(self.data.get("hoverboard_stock", 3))
        u.outfit_id = int(self.data.get("outfit_id", 0))
        return u
