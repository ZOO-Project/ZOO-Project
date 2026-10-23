# -*- coding: utf-8 -*-
#
# Written in 2026 by gerald.fenoy at geolabs.fr
#
# To the extent possible under law, the
# author(s) have dedicated all copyright and
# related and neighboring rights to this
# software to the public domain worldwide. This
# software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain
# Dedication along with this software. If not, see
# http://creativecommons.org/publicdomain/zero/1.0/.
#
import os

import yaml
import zoo


class _CWLDumper(yaml.SafeDumper):
    """Writes multi-line strings as literal block scalars (|),
    including when some lines end with spaces."""

    def analyze_scalar(self, scalar):
        analysis = super().analyze_scalar(scalar)
        if (
            not analysis.allow_block
            and "\n" in scalar
            and not scalar.endswith(" ")
            and all(ch == "\n" or ch.isprintable() for ch in scalar)
        ):
            analysis.allow_block = True
        return analysis


def _str_presenter(dumper, data):
    if "\n" in data:
        return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")
    return dumper.represent_scalar("tag:yaml.org,2002:str", data)


_CWLDumper.add_representer(str, _str_presenter)


class CWLGraph:
    """Assemble a CWL document and its dependencies (run: file.cwl) into a single $graph document.

    - The main process is the first element of the $graph.
    - The main CWL document's metadata (prefixed keys, e.g. s:author) and its
        cwlVersion are placed at the root of the generated document.
    """

    @staticmethod
    def _id_from_path(path):
        return os.path.splitext(os.path.basename(path))[0]

    @staticmethod
    def _is_metadata(key):
        # CWL extension fields: prefixed by a namespace (s:author) or a full URI
        return ":" in key and not key.startswith("$")

    @staticmethod
    def dump(packed, stream=None):
        text = yaml.dump(
            packed,
            Dumper=_CWLDumper,
            sort_keys=False,
            allow_unicode=True,
            width=4096,
        )
        if yaml.safe_load(text) != packed:
            raise ValueError(
                "YAML serialization does not preserve the document exactly"
            )
        if stream is None:
            return text
        stream.write(text)

    @classmethod
    def _handle_comments(cls, node, mode):
        """Recursively remove or convert $comment keys. Return the number processed."""
        count = 0
        if isinstance(node, dict):
            if "$comment" in node:
                comment = node.pop("$comment")
                if mode == "rdfs":
                    node["rdfs:comment"] = comment
                count += 1
            for value in node.values():
                count += cls._handle_comments(value, mode)
        elif isinstance(node, list):
            for value in node:
                count += cls._handle_comments(value, mode)
        return count

    @classmethod
    def pack(cls, cwl_path, root_id=None, comments="drop"):
        root_path = os.path.abspath(cwl_path)
        acc = {"cwlVersion": None, "$namespaces": {}, "$schemas": [], "$graph": []}
        metadata = cls._load(root_path, root_id, acc, {root_path}, is_root=True)

        acc["$graph"].insert(0, acc["$graph"].pop())

        handled = cls._handle_comments(acc["$graph"], comments)
        handled += cls._handle_comments(metadata, comments)
        if handled and comments == "rdfs":
            acc["$namespaces"].setdefault(
                "rdfs", "http://www.w3.org/2000/01/rdf-schema#"
            )

        packed = {"cwlVersion": acc["cwlVersion"] or "v1.2"}
        if acc["$namespaces"]:
            packed["$namespaces"] = acc["$namespaces"]
        if acc["$schemas"]:
            packed["$schemas"] = acc["$schemas"]
        packed.update(metadata)
        packed["$graph"] = acc["$graph"]
        return packed

    @classmethod
    def _load(cls, cwl_path, cwl_id, acc, seen, is_root=False):
        with open(cwl_path, encoding="utf-8") as f:
            doc = yaml.safe_load(f)

        if "$graph" in doc:  # already packed file
            graph = doc.pop("$graph")
            process = next(
                (e for e in graph if str(e.get("id", "")).lstrip("#") == "main"),
                graph[0],
            )
            doc_level = doc  # remaining document metadata outside $graph
        else:
            process, doc_level = doc, {}

        for src in (doc_level, process):
            acc["$namespaces"].update(src.pop("$namespaces", None) or {})
            for schema in src.pop("$schemas", None) or []:
                if schema not in acc["$schemas"]:
                    acc["$schemas"].append(schema)
            version = src.pop("cwlVersion", None)
            if is_root and version:
                acc["cwlVersion"] = version

        metadata = {}
        if is_root:
            for src in (doc_level, process):
                for key in [k for k in src if cls._is_metadata(k)]:
                    metadata[key] = src.pop(key)
            cwl_id = (
                cwl_id
                or str(process.get("id", "")).lstrip("#")
                or cls._id_from_path(cwl_path)
            )
        process["id"] = cwl_id

        steps = process.get("steps", {})
        step_list = steps.values() if isinstance(steps, dict) else steps
        base_dir = os.path.dirname(cwl_path)

        for step in step_list:
            run = step.get("run")
            if not isinstance(run, str) or run.startswith("#"):
                continue
            step_path = os.path.normpath(os.path.join(base_dir, run))
            step_id = cls._id_from_path(step_path)
            if step_path not in seen:
                seen.add(step_path)
                cls._load(step_path, step_id, acc, seen)
            step["run"] = "#" + step_id

        acc["$graph"].append(process)
        return metadata


def cwl_pack(conf, inputs, outputs):
    try:
        zoo.info(str(conf["cwl-parts"]))
        zoo.info(str(conf["lenv"]))
        packed = CWLGraph.pack(conf["cwl-parts"]["path"])
        with open(
            conf["main"]["tmpPath"]
            + "/app-pkg-packed-"
            + conf["lenv"]["usid"]
            + ".cwl",
            "w",
            encoding="utf-8",
        ) as f:
            CWLGraph.dump(packed, f)
        conf["renv"]["jrequest"] = open(
            conf["main"]["tmpPath"]
            + "/app-pkg-packed-"
            + conf["lenv"]["usid"]
            + ".cwl",
            "r",
            encoding="utf-8",
        ).read()
        return zoo.SERVICE_SUCCEEDED
    except Exception as e:
        zoo.info(str(e))
        return zoo.SERVICE_FAILED
