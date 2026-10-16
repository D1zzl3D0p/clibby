"""Anatomy constraints for the editable production recipes."""
import unittest

from tools.author_characters import soft_creature, werewolf


class AnatomyTests(unittest.TestCase):
    def test_soft_knees_flex_forward_together_without_broad_splay(self):
        character = soft_creature().manifest
        bones = {bone['id']: bone['rest'] for bone in character['bones']}
        knees = [bones['knee-' + side] for side in ('l', 'r')]
        assert all(0 < knee['x'] < knee['y'] / 2 for knee in knees)
        assert all(leg['bend'] == -1 for leg in character['locomotion']['legs'])
        for side in ('l', 'r'):
            assert bones['knee-' + side]['x'] + bones['foot-' + side]['x'] == 0


    def test_both_production_bipeds_use_grounded_world_contacts(self):
        for recipe in (soft_creature, werewolf):
            character = recipe().manifest
            locomotion = character['locomotion']
            assert len(locomotion['legs']) == 2
            assert {leg['ankle'] for leg in locomotion['legs']} == {
                contact['bone'] for contact in character['contacts']}
            world = {}
            for bone in character['bones']:
                parent = world.get(bone['parent'], (0, 0))
                rest = bone['rest']
                world[bone['id']] = (parent[0] + rest['x'], parent[1] + rest['y'])
            for leg in locomotion['legs']:
                assert world[leg['ankle']][1] + leg['contact'][1] == locomotion['groundY']
