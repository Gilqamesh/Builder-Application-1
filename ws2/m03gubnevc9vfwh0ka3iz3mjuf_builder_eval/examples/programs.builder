(define hello "hello from a file")

(define simple-list
  (list "parse" "evaluate" "publish"))

(define simple-record-field
  (get
    (record "module" "builder_eval" "mode" "file")
    "mode"))

(define scoped-record-field
  (let ((result (record "message" "file-record-ok")))
    (begin
      (record? result)
      (get result "message"))))

(define choose-message
  (lambda (ready)
    (if ready "ready" "not-ready")))

(define chosen-message
  (choose-message (equal? simple-record-field "file")))

(define make-status
  (lambda (name ready)
    (record
      "name" name
      "state" (choose-message ready))))

(define nested-status
  (let ((status (make-status "builder_eval" true)))
    (record
      "summary" (get status "state")
      "details" status
      "steps" simple-list)))

(define workspace-exists
  (let ((fs (filesystem)) (kernel-api (kernel)))
    ((get fs "exists") ((get kernel-api "workspace_root")))))

(define serialized-capability-call
  (let ((json-api (json)) (fs (filesystem)))
    (((get json-api "deserialize_value")
      ((get json-api "serialize_value") (get fs "exists")))
     ((get fs "current_path")))))

(record
  "01_literal" hello
  "02_list" simple-list
  "03_record_get" simple-record-field
  "04_let_begin_record_get" scoped-record-field
  "05_lambda_if_equal" chosen-message
  "06_nested_record" nested-status
  "07_native_workspace_exists" workspace-exists
  "08_serialized_capability_call" serialized-capability-call)
