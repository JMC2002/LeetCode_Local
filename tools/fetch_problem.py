#!/usr/bin/env python3
"""按题号从 LeetCode 拉取题目模板和题面样例。"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tempfile
import time
from html.parser import HTMLParser
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


WORKSPACE = Path(__file__).resolve().parents[1]
DEFAULT_PROBLEMS_DIR = WORKSPACE / "problems"
CACHE_DIR = WORKSPACE / ".cache" / "leetcode"
INDEX_MAX_AGE_SECONDS = 24 * 60 * 60

SITES = {
    "cn": "https://leetcode.cn",
    "com": "https://leetcode.com",
}

USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/138.0.0.0 Safari/537.36"
)

QUESTION_QUERY = r"""
query questionData($titleSlug: String!) {
  question(titleSlug: $titleSlug) {
    questionFrontendId
    title
    titleSlug
    translatedTitle
    content
    translatedContent
    isPaidOnly
    exampleTestcaseList
    sampleTestCase
    metaData
    codeSnippets {
      lang
      langSlug
      code
    }
  }
}
"""


class FetchError(RuntimeError):
    """可直接展示给用户的拉取错误。"""


class VisibleTextParser(HTMLParser):
    """把题面 HTML 转为保留标签边界的可见文本。"""

    _line_break_tags = {
        "br",
        "div",
        "h1",
        "h2",
        "h3",
        "h4",
        "h5",
        "h6",
        "li",
        "ol",
        "p",
        "pre",
        "strong",
        "table",
        "td",
        "th",
        "tr",
        "ul",
    }

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self._parts: list[str] = []

    def handle_starttag(
        self, tag: str, attrs: list[tuple[str, str | None]]
    ) -> None:
        del attrs
        if tag in self._line_break_tags:
            self._parts.append("\n")

    def handle_endtag(self, tag: str) -> None:
        if tag in self._line_break_tags:
            self._parts.append("\n")

    def handle_data(self, data: str) -> None:
        self._parts.append(data)

    def text(self) -> str:
        return "".join(self._parts)


def warn(message: str) -> None:
    print(f"警告：{message}", file=sys.stderr)


def atomic_write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary.write(content)
            temporary_name = temporary.name
        os.replace(temporary_name, path)
    finally:
        if temporary_name is not None:
            try:
                Path(temporary_name).unlink(missing_ok=True)
            except OSError:
                pass


def request_json(
    url: str,
    *,
    timeout: float,
    referer: str,
    payload: dict[str, Any] | None = None,
) -> dict[str, Any]:
    body = None
    headers = {
        "Accept": "application/json, text/plain, */*",
        "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.8",
        "Referer": referer,
        "User-Agent": USER_AGENT,
    }
    if payload is not None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        headers["Content-Type"] = "application/json; charset=utf-8"
        origin_match = re.match(r"https://[^/]+", url)
        if origin_match:
            headers["Origin"] = origin_match.group(0)

    request = Request(url, data=body, headers=headers, method="POST" if body else "GET")
    try:
        with urlopen(request, timeout=timeout) as response:
            raw = response.read()
    except HTTPError as error:
        detail = error.read(512).decode("utf-8", errors="replace")
        if error.code == 403 and ("Just a moment" in detail or "cf_chl" in detail):
            raise FetchError(f"{url} 被 Cloudflare 拒绝（HTTP 403）") from error
        raise FetchError(f"请求 {url} 失败：HTTP {error.code}") from error
    except URLError as error:
        raise FetchError(f"请求 {url} 失败：{error.reason}") from error
    except TimeoutError as error:
        raise FetchError(f"请求 {url} 超时") from error

    try:
        result = json.loads(raw)
    except json.JSONDecodeError as error:
        raise FetchError(f"{url} 没有返回有效 JSON") from error
    if not isinstance(result, dict):
        raise FetchError(f"{url} 返回的 JSON 顶层不是对象")
    return result


def cache_path(site: str) -> Path:
    return CACHE_DIR / f"{site}-problem-index.json"


def parse_problem_index(payload: dict[str, Any]) -> dict[str, str]:
    result: dict[str, str] = {}
    pairs = payload.get("stat_status_pairs")
    if not isinstance(pairs, list):
        raise FetchError("题目列表响应缺少 stat_status_pairs")

    for pair in pairs:
        if not isinstance(pair, dict):
            continue
        stat = pair.get("stat")
        if not isinstance(stat, dict):
            continue
        frontend_id = str(stat.get("frontend_question_id", "")).strip()
        slug = stat.get("question__title_slug")
        if frontend_id and isinstance(slug, str) and slug:
            result[frontend_id] = slug

    if not result:
        raise FetchError("题目列表响应中没有可用题目")
    return result


def load_cached_index(site: str, *, allow_stale: bool) -> dict[str, str] | None:
    path = cache_path(site)
    try:
        age = time.time() - path.stat().st_mtime
        if not allow_stale and age > INDEX_MAX_AGE_SECONDS:
            return None
        payload = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(payload, dict):
            return None
        items = payload.get("items")
        if not isinstance(items, dict):
            return None
        return {
            str(frontend_id): str(slug)
            for frontend_id, slug in items.items()
            if str(frontend_id) and str(slug)
        }
    except (OSError, json.JSONDecodeError, TypeError, ValueError):
        return None


def get_problem_index(
    site: str,
    *,
    timeout: float,
    use_cache: bool,
    refresh: bool,
) -> dict[str, str]:
    if use_cache and not refresh:
        cached = load_cached_index(site, allow_stale=False)
        if cached is not None:
            return cached

    base = SITES[site]
    try:
        payload = request_json(
            f"{base}/api/problems/all/",
            timeout=timeout,
            referer=f"{base}/problemset/",
        )
        result = parse_problem_index(payload)
        if use_cache:
            serialized = json.dumps(
                {"site": site, "updated_at": int(time.time()), "items": result},
                ensure_ascii=False,
                indent=2,
                sort_keys=True,
            )
            atomic_write_text(cache_path(site), serialized + "\n")
        return result
    except FetchError:
        if use_cache:
            stale = load_cached_index(site, allow_stale=True)
            if stale is not None:
                warn(f"{site} 站题目索引更新失败，使用本地旧缓存")
                return stale
        raise


def preferred_sites(selection: str) -> list[str]:
    if selection == "auto":
        return ["cn", "com"]
    return [selection]


def resolve_slug(
    frontend_id: int,
    *,
    selection: str,
    timeout: float,
    use_cache: bool,
    refresh: bool,
) -> tuple[str, str]:
    failures: list[str] = []
    key = str(frontend_id)
    for site in preferred_sites(selection):
        try:
            index = get_problem_index(
                site,
                timeout=timeout,
                use_cache=use_cache,
                refresh=refresh,
            )
        except FetchError as error:
            failures.append(str(error))
            continue
        if key in index:
            if failures:
                warn(f"中国站不可用，已回退到 {SITES[site]}")
            return index[key], site
        failures.append(f"{SITES[site]} 的题目列表中没有题号 {frontend_id}")
    raise FetchError("；".join(failures))


def fetch_question(
    frontend_id: int,
    slug: str,
    *,
    selection: str,
    timeout: float,
) -> tuple[dict[str, Any], str]:
    failures: list[str] = []
    for site in preferred_sites(selection):
        base = SITES[site]
        try:
            response = request_json(
                f"{base}/graphql/",
                timeout=timeout,
                referer=f"{base}/problems/{slug}/",
                payload={
                    "operationName": "questionData",
                    "query": QUESTION_QUERY,
                    "variables": {"titleSlug": slug},
                },
            )
        except FetchError as error:
            failures.append(str(error))
            continue

        errors = response.get("errors")
        if errors:
            failures.append(f"{base}/graphql/ 返回错误：{errors}")
            continue
        data = response.get("data")
        question = data.get("question") if isinstance(data, dict) else None
        if not isinstance(question, dict):
            failures.append(f"{base}/graphql/ 没有返回题目 {slug}")
            continue
        returned_id = str(question.get("questionFrontendId", ""))
        if returned_id != str(frontend_id):
            failures.append(
                f"{base}/graphql/ 返回题号 {returned_id or '未知'}，预期 {frontend_id}"
            )
            continue
        if failures:
            warn(f"中国站题目详情不可用，已回退到 {base}")
        return question, site

    raise FetchError("；".join(failures))


def extract_outputs(content: str) -> list[str]:
    parser = VisibleTextParser()
    parser.feed(content)
    parser.close()
    visible = parser.text().replace("\r", "").replace("\xa0", " ")
    lines = [line.strip() for line in visible.split("\n")]

    output_label = re.compile(r"^(?:Output|输出)\s*[：:]?\s*(.*)$", re.IGNORECASE)
    stop_label = re.compile(
        r"^(?:Input|输入|Explanation|解释|说明|Example|示例|Constraints|提示)\s*[：:]?",
        re.IGNORECASE,
    )

    outputs: list[str] = []
    index = 0
    while index < len(lines):
        match = output_label.match(lines[index])
        if match is None:
            index += 1
            continue

        parts: list[str] = []
        first = match.group(1).strip()
        if first:
            parts.append(first)
        index += 1
        while index < len(lines):
            line = lines[index]
            if stop_label.match(line) or output_label.match(line):
                break
            if line:
                parts.append(line)
            index += 1
        value = "\n".join(parts).strip()
        if value:
            if value == "True":
                value = "true"
            elif value == "False":
                value = "false"
            outputs.append(value)

    return outputs


def example_inputs(question: dict[str, Any]) -> list[str]:
    cases = question.get("exampleTestcaseList")
    if isinstance(cases, list):
        result = [case.strip() for case in cases if isinstance(case, str) and case.strip()]
        if result:
            return result
    sample = question.get("sampleTestCase")
    if isinstance(sample, str) and sample.strip():
        return [sample.strip()]
    raise FetchError("题目没有返回可用的 exampleTestcaseList 或 sampleTestCase")


def make_cases(
    inputs: list[str], outputs: list[str], *, inputs_only: bool
) -> tuple[str, bool]:
    include_outputs = not inputs_only and len(outputs) == len(inputs)
    if not inputs_only and not include_outputs:
        warn(
            f"题面中提取到 {len(outputs)} 个输出，但有 {len(inputs)} 个样例；"
            "本次只写入输入"
        )

    records: list[str] = []
    for position, case_input in enumerate(inputs):
        record = case_input.rstrip()
        if include_outputs:
            record += f"\n=> {outputs[position]}"
        records.append(record)
    return "\n\n".join(records) + "\n", include_outputs


def sanitize_title(title: str) -> str:
    result = re.sub(r'[<>:"/\\|?*\x00-\x1f]', "_", title).strip().rstrip(".")
    result = re.sub(r"\s+", " ", result)
    return result or "Untitled"


def find_problem_directory(problems_dir: Path, frontend_id: int) -> Path | None:
    matches = sorted(
        path
        for path in problems_dir.glob(f"{frontend_id}.*")
        if path.is_dir()
    )
    if len(matches) > 1:
        joined = ", ".join(path.name for path in matches)
        raise FetchError(f"题号 {frontend_id} 对应多个目录：{joined}")
    return matches[0] if matches else None


def cpp_template(question: dict[str, Any]) -> str:
    snippets = question.get("codeSnippets")
    if isinstance(snippets, list):
        for snippet in snippets:
            if not isinstance(snippet, dict):
                continue
            lang_slug = str(snippet.get("langSlug", "")).lower()
            lang = str(snippet.get("lang", "")).lower()
            code = snippet.get("code")
            if isinstance(code, str) and code.strip() and (
                lang_slug == "cpp" or lang in {"c++", "cpp"}
            ):
                return "#include <lc/prelude.hpp>\n\n" + code.strip() + "\n"

    return (
        "#include <lc/prelude.hpp>\n\n"
        "class Solution {\n"
        "public:\n"
        "    // TODO: 根据题目补充方法。\n"
        "};\n"
    )


def choose_title(question: dict[str, Any]) -> str:
    translated = question.get("translatedTitle")
    original = question.get("title")
    if isinstance(translated, str) and translated.strip():
        return translated.strip()
    if isinstance(original, str) and original.strip():
        return original.strip()
    raise FetchError("题目详情没有返回标题")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "id",
        type=int,
        nargs="?",
        help="十进制题号；省略时读取 current_problem.txt",
    )
    parser.add_argument(
        "--site",
        choices=("auto", "cn", "com"),
        default="auto",
        help="数据站点；auto 会优先中国站并回退国际站（默认：auto）",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="覆盖已有 cases.txt；不会覆盖 solution.cpp",
    )
    parser.add_argument(
        "--inputs-only",
        action="store_true",
        help="只写样例输入，不从题面提取期望输出",
    )
    parser.add_argument(
        "--select",
        action="store_true",
        help="成功后把题号写入 current_problem.txt",
    )
    parser.add_argument(
        "--refresh-index",
        action="store_true",
        help="忽略 24 小时内的题号索引缓存",
    )
    parser.add_argument(
        "--no-cache",
        action="store_true",
        help="不读取或写入题号索引缓存",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=20.0,
        help="单次网络请求超时秒数（默认：20）",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="完成请求和解析，但不创建或修改题目文件",
    )
    return parser.parse_args()


def selected_problem_id() -> int:
    selection_path = WORKSPACE / "current_problem.txt"
    try:
        value = selection_path.read_text(encoding="utf-8").strip()
    except OSError as error:
        raise FetchError(f"无法读取 {selection_path}：{error}") from error
    if not re.fullmatch(r"[0-9]+", value):
        raise FetchError(f"current_problem.txt 只能包含十进制题号：{value!r}")
    result = int(value)
    if result <= 0:
        raise FetchError("current_problem.txt 中的题号必须是正整数")
    return result


def run(args: argparse.Namespace) -> int:
    frontend_id = args.id if args.id is not None else selected_problem_id()
    if frontend_id <= 0:
        raise FetchError("题号必须是正整数")
    if args.timeout <= 0:
        raise FetchError("--timeout 必须大于 0")

    slug, _ = resolve_slug(
        frontend_id,
        selection=args.site,
        timeout=args.timeout,
        use_cache=not args.no_cache,
        refresh=args.refresh_index,
    )
    question, detail_site = fetch_question(
        frontend_id,
        slug,
        selection=args.site,
        timeout=args.timeout,
    )
    title = choose_title(question)
    inputs = example_inputs(question)
    content = question.get("translatedContent") or question.get("content") or ""
    outputs = extract_outputs(content) if isinstance(content, str) else []
    cases_content, included_outputs = make_cases(
        inputs, outputs, inputs_only=args.inputs_only
    )

    existing = find_problem_directory(DEFAULT_PROBLEMS_DIR, frontend_id)
    target = existing or DEFAULT_PROBLEMS_DIR / f"{frontend_id}. {sanitize_title(title)}"

    print(f"题目：{frontend_id}. {title}")
    print(f"网址：{SITES[detail_site]}/problems/{slug}/")
    print(
        f"样例：{len(inputs)} 个"
        + ("，包含期望输出" if included_outputs else "，仅输入")
    )
    print(f"目录：{target}")

    if args.dry_run:
        print("演练完成：没有修改文件")
        return 0

    target.mkdir(parents=True, exist_ok=True)
    solution_path = target / "solution.cpp"
    cases_path = target / "cases.txt"

    if not solution_path.exists():
        atomic_write_text(solution_path, cpp_template(question))
        print(f"已创建：{solution_path}")
    else:
        print(f"已保留：{solution_path}")

    if cases_path.exists() and not args.force:
        print(f"已保留：{cases_path}（使用 --force 才会覆盖）")
    else:
        atomic_write_text(cases_path, cases_content)
        print(f"已写入：{cases_path}")

    if args.select:
        selection_path = WORKSPACE / "current_problem.txt"
        atomic_write_text(selection_path, str(frontend_id))
        print(f"已选择：{frontend_id}")

    return 0


def main() -> int:
    args = parse_arguments()
    try:
        return run(args)
    except (FetchError, OSError) as error:
        print(f"错误：{error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
