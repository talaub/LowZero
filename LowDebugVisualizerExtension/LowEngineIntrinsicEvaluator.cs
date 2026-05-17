using System;
using System.Collections.ObjectModel;
using System.Threading;
using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Evaluation.IL;

namespace LowEngine.DebugVisualizerExtension
{
    public sealed class LowEngineIntrinsicEvaluator : IDkmIntrinsicFunctionEvaluator140
    {
        private static readonly Guid SourceId = new Guid("8d6a4ce8-f35a-41e9-bf1e-5b51f4ecb5b1");

        private const uint IntrinsicNameDebugString = 1001;
        private const uint IntrinsicHandleDebugString = 1101;
        private const uint IntrinsicHandleTypeString = 1102;
        private const uint IntrinsicHandleLivenessString = 1103;
        private const uint IntrinsicHandleNameString = 1104;

        public DkmILEvaluationResult[] Execute(
            DkmILExecuteIntrinsic executeIntrinsic,
            DkmILContext iLContext,
            DkmCompiledILInspectionQuery inspectionQuery,
            DkmILEvaluationResult[] arguments,
            ReadOnlyCollection<DkmCompiledInspectionQuery> subroutines,
            out DkmILFailureReason failureReason)
        {
            failureReason = DkmILFailureReason.None;

            try
            {
                if (arguments == null || arguments.Length == 0)
                {
                    return ReturnPointer(0);
                }

                ulong argument = ReadPointer(arguments[0].ResultBytes);
                switch (executeIntrinsic.Id)
                {
                    case IntrinsicNameDebugString:
                        return ReturnPointer(GetNameStringPointer(iLContext, inspectionQuery, argument));
                    case IntrinsicHandleDebugString:
                        return ReturnPointer(GetHandleStringPointer(iLContext, inspectionQuery, argument, "debug_string"));
                    case IntrinsicHandleTypeString:
                        return ReturnPointer(GetHandleStringPointer(iLContext, inspectionQuery, argument, "debug_type_string"));
                    case IntrinsicHandleLivenessString:
                        return ReturnPointer(GetHandleStringPointer(iLContext, inspectionQuery, argument, "debug_liveness_string"));
                    case IntrinsicHandleNameString:
                        return ReturnPointer(GetHandleStringPointer(iLContext, inspectionQuery, argument, "debug_name_string"));
                    default:
                        failureReason = DkmILFailureReason.UnknownFuncEvalError;
                        return Array.Empty<DkmILEvaluationResult>();
                }
            }
            catch
            {
                failureReason = DkmILFailureReason.UnknownFuncEvalError;
                return Array.Empty<DkmILEvaluationResult>();
            }
        }

        private static ulong GetNameStringPointer(
            DkmILContext iLContext,
            DkmCompiledILInspectionQuery inspectionQuery,
            ulong nameAddress)
        {
            if (nameAddress == 0)
            {
                return 0;
            }

            string expression = "(unsigned __int64)((Low::Util::Name*)0x" +
                nameAddress.ToString("x") + ")->debug_c_str()";
            return TryEvaluatePointerExpression(iLContext, inspectionQuery, expression);
        }

        private static ulong GetHandleStringPointer(
            DkmILContext iLContext,
            DkmCompiledILInspectionQuery inspectionQuery,
            ulong handleAddress,
            string methodName)
        {
            if (handleAddress == 0)
            {
                return 0;
            }

            string expression = "(unsigned __int64)((Low::Util::Handle*)0x" +
                handleAddress.ToString("x") + ")->" + methodName + "()";
            return TryEvaluatePointerExpression(iLContext, inspectionQuery, expression);
        }

        private static ulong TryEvaluatePointerExpression(
            DkmILContext iLContext,
            DkmCompiledILInspectionQuery inspectionQuery,
            string expressionText)
        {
            try
            {
                var success =
                    TryEvaluateExpression(iLContext, inspectionQuery, expressionText);
                if (success == null)
                {
                    return 0;
                }

                return ParsePointer(success.Value);
            }
            catch (Exception e)
            {
                _ = e;
                return 0;
            }
        }

        private static DkmSuccessEvaluationResult TryEvaluateExpression(
            DkmILContext iLContext,
            DkmCompiledILInspectionQuery inspectionQuery,
            string expressionText)
        {
            DkmLanguage language = DkmLanguage.Create("C++", inspectionQuery.LanguageId);
            DkmInspectionSession session = DkmInspectionSession.Create(iLContext.StackFrame.Process, null);
            DkmInspectionContext context = DkmInspectionContext.Create(
                session,
                inspectionQuery.RuntimeInstance,
                iLContext.StackFrame.Thread,
                5000,
                DkmEvaluationFlags.TreatAsExpression |
                    DkmEvaluationFlags.ForceEvaluationNow |
                    DkmEvaluationFlags.ForceRealFuncEval |
                    DkmEvaluationFlags.EnableExtendedSideEffects,
                DkmFuncEvalFlags.None,
                10,
                language,
                null);

            var expression = DkmLanguageExpression.Create(
                language,
                DkmEvaluationFlags.TreatAsExpression,
                expressionText,
                null);

            DkmEvaluateExpressionAsyncResult result = default(DkmEvaluateExpressionAsyncResult);
            bool hasResult = false;
            Exception exception = null;
            using (var completed = new ManualResetEventSlim(false))
            {
                DkmWorkList workList = DkmWorkList.Create(delegate
                {
                    completed.Set();
                });

                context.EvaluateExpression(
                    workList,
                    expression,
                    iLContext.StackFrame,
                    delegate (DkmEvaluateExpressionAsyncResult asyncResult)
                    {
                        result = asyncResult;
                        hasResult = true;
                    });

                try
                {
                    workList.Execute();
                }
                catch (Exception e)
                {
                    exception = e;
                }

                if (!completed.Wait(5000))
                {
                    return null;
                }
            }

            if (exception != null || !hasResult || result.ErrorCode != 0)
            {
                return null;
            }

            return result.ResultObject as DkmSuccessEvaluationResult;
        }

        private static ulong ParsePointer(string value)
        {
            if (String.IsNullOrEmpty(value))
            {
                return 0;
            }

            value = value.Trim();
            int spaceIndex = value.IndexOf(' ');
            if (spaceIndex > 0)
            {
                value = value.Substring(0, spaceIndex);
            }

            if (value.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            {
                return Convert.ToUInt64(value.Substring(2), 16);
            }

            return Convert.ToUInt64(value, 10);
        }

        private static DkmILEvaluationResult[] ReturnPointer(ulong pointer)
        {
            byte[] pointerBytes = BitConverter.GetBytes(pointer);
            var result = DkmILEvaluationResult.Create(
                SourceId,
                new ReadOnlyCollection<byte>(pointerBytes),
                false,
                null);

            return new[] { result };
        }

        private static ulong ReadPointer(ReadOnlyCollection<byte> bytes)
        {
            if (bytes.Count >= 8)
            {
                return BitConverter.ToUInt64(ToArray(bytes, 8), 0);
            }

            if (bytes.Count >= 4)
            {
                return BitConverter.ToUInt32(ToArray(bytes, 4), 0);
            }

            return 0;
        }

        private static byte[] ToArray(ReadOnlyCollection<byte> bytes, int count)
        {
            byte[] result = new byte[count];
            for (int i = 0; i < count && i < bytes.Count; ++i)
            {
                result[i] = bytes[i];
            }

            return result;
        }
    }
}
