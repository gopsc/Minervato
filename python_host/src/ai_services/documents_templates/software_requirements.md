# {{ document_type }}

{{ title }}

## 项目信息

**项目名称**: {{ project_info.project_name }}
**版本**: {{ project_info.version }}
**作者**: {{ project_info.author }}
**创建日期**: {{ project_info.creation_date }}

## 软件需求

### 需求列表

{% for req in software_requirements %}
### 需求 {{ loop.index }}: {{ req.id }} - {{ req.name }}
**描述**: {{ req.description }}
**优先级**: {{ req.priority }}
**类别**: {{ req.category }}
**验收标准**: {{ req.acceptance_criteria }}
**预估工时**: {{ req.duration.estimation }}
**状态**: {{ req.status }}

{% endfor %}

## 时间线

**项目开始日期**: {{ timeline.project_start_date }}
**项目结束日期**: {{ timeline.project_end_date }}
**总工期**: {{ timeline.total_duration }}