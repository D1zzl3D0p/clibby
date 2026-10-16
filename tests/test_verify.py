import copy
import unittest

try:
    from tools.verify import compare_pose_documents
except ImportError:
    compare_pose_documents = None


class VerificationTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(compare_pose_documents, "pose comparator must exist")
        self.document = {"version": 1, "poses": [{"scenario": "static", "tick": 0,
            "root_offset": [0, 0], "parts": [{"id": "body", "x": 1, "y": 2,
            "rotation": 0, "scale_x": 1, "scale_y": 1}]}]}

    def test_roundoff_is_accepted_but_drift_is_not(self):
        actual = copy.deepcopy(self.document)
        actual["poses"][0]["parts"][0]["rotation"] = 0.00001
        self.assertEqual(compare_pose_documents(self.document, actual), [])
        actual["poses"][0]["parts"][0]["rotation"] = 0.01
        self.assertTrue(compare_pose_documents(self.document, actual))

    def test_nonfinite_result_is_rejected(self):
        actual = copy.deepcopy(self.document)
        actual["poses"][0]["root_offset"][0] = float("nan")
        self.assertTrue(compare_pose_documents(self.document, actual))

    def test_missing_pose_and_part_are_rejected(self):
        self.assertTrue(compare_pose_documents(self.document, {"version": 1, "poses": []}))
        actual = copy.deepcopy(self.document)
        actual["poses"][0]["parts"] = []
        self.assertTrue(compare_pose_documents(self.document, actual))

    def test_wrong_scenario_is_rejected(self):
        actual = copy.deepcopy(self.document)
        actual["poses"][0]["scenario"] = "hidden"
        self.assertTrue(compare_pose_documents(self.document, actual))
