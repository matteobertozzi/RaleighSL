class rpc_rusage_struct(FieldStruct):
    _FIELDS = {
         1: ('utime',        'uint',  None),
         2: ('stime',        'uint',  None),
         3: ('maxrss',       'uint',  None),
         4: ('minflt',       'uint',  None),
         5: ('majflt',       'uint',  None),
         6: ('inblock',      'uint',  None),
         7: ('oublock',      'uint',  None),
         8: ('nvcsw',        'uint',  None),
         9: ('nivcsw',       'uint',  None),
        10: ('iowait',       'uint',  None),
        11: ('ioread',       'uint',  None),
        12: ('iowrite',      'uint',  None),
    }
