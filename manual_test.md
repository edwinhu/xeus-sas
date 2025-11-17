# Manual Test for Persistent SAS Session

## Steps to Test

1. Launch euporie console:
   ```bash
   euporie console xeus-sas
   ```

2. In the first cell, create a dataset:
   ```sas
   DATA work.mydata;
       INPUT x y;
       DATALINES;
   1 10
   2 20
   3 30
   ;
   RUN;
   ```

3. **Check the output** - you should see:
   - "Initializing persistent SAS session..." (ONCE)
   - "Persistent SAS session initialized (PID: XXXX)" (ONCE)
   - SAS log output

4. In a second cell, print the dataset:
   ```sas
   PROC PRINT DATA=work.mydata;
   RUN;
   ```

5. **Check the output** - you should see:
   - **NO "SAS initialization used" messages** (this is the key!)
   - The data printed (proving work.mydata persisted)

6. In a third cell, run a simple command:
   ```sas
   %put Hello from persistent session!;
   ```

7. **Check the output** - you should see:
   - **NO "SAS initialization used" messages**
   - Just "Hello from persistent session!"

## Expected Behavior

**Before (Batch Mode)**:
- Every execution showed "NOTE: SAS initialization used"
- Datasets didn't persist between executions

**After (Persistent Session)**:
- SAS initializes ONCE at first execution
- NO initialization messages for subsequent executions
- Datasets, macros, and libraries persist throughout the session
- Single PID for the SAS process (can verify with `ps aux | grep sas`)

## Success Criteria

✓ Session initialization happens once
✓ No initialization messages on subsequent code executions
✓ Datasets persist between cells
✓ Same SAS PID throughout the session
