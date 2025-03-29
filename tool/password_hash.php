#!/usr/bin/env php
<?= password_hash(rtrim(stream_get_contents(STDIN), "\n"), PASSWORD_DEFAULT);