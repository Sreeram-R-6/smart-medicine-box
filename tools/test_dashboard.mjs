import * as fs from 'node:fs/promises';
import * as vm from 'node:vm';
import { checkDashboard } from '../tests/dashboard_test.mjs';

console.log(await checkDashboard(fs, vm));
